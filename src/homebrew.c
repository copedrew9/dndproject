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

/* Written at the top of homebrew.txt every time it is saved, and shipped as
   the file's initial contents, so the format is documented where a DM will
   actually look for it. The example is commented out: uncomment a line to
   add that entry. */
static const char HOMEBREW_HEADER[] =
"# homebrew.txt -- your own items and spells.\n"
"#\n"
"# This is the one data file the program reads while it is running. Add a\n"
"# line here and the entry appears everywhere a printed one does: in the\n"
"# shops, in the lookup browser, in a spell list, on a sheet. Entries made\n"
"# through the Homebrew menu are written back here, so you can start\n"
"# either way.\n"
"#\n"
"# One record per line: a tag, then '|' separated fields. A line starting\n"
"# with '#' is a comment. Everything here is tagged as Homebrew, so the\n"
"# settings menu can switch it all off at once.\n"
"#\n"
"# ITEM|name|category|cost_cp|weight_tenths|base_ac|dex_cap|str_req|\n"
"#      stealth|damage|damage_type|properties|contents\n"
"#   category is one of: light-armour, medium-armour, heavy-armour,\n"
"#      shield, simple-melee, simple-ranged, martial-melee,\n"
"#      martial-ranged, gear, tool, pack, mount\n"
"#   cost is in copper, so 25 gp is 2500. Weight is in tenths of a pound,\n"
"#      so 3 lb is 30.\n"
"#   base_ac, dex_cap, str_req and stealth are for armour and are 0 on\n"
"#      anything else. dex_cap of -1 adds the whole Dexterity modifier.\n"
"#   damage and properties are for weapons; contents is for a pack.\n"
"#\n"
"# MAGICITEM|name|type|rarity|attunement|text\n"
"#   attunement is blank when none is needed, otherwise the wording, e.g.\n"
"#      'requires attunement by a druid'.\n"
"#\n"
"# SPELL|name|level|school|ritual|concentration|casting_time|range|\n"
"#       components|duration|classes\n"
"#   level 0 is a cantrip. school is one of: Abjuration, Conjuration,\n"
"#      Divination, Enchantment, Evocation, Illusion, Necromancy,\n"
"#      Transmutation.\n"
"#   ritual and concentration are 1 or 0.\n"
"#   classes is a comma separated list of the classes whose list it is on:\n"
"#      bard, cleric, druid, paladin, ranger, sorcerer, warlock, wizard,\n"
"#      artificer. A spell nobody can learn is one nobody will see.\n"
"#\n"
"# An example of each. Delete the '#' from a line to bring it into play.\n"
"#\n"
"#ITEM|Dwarven Repeating Crossbow|martial-ranged|7500|180|0|0|0|0|1d10|"
"piercing|Ammunition (range 100/400), heavy, loading, two-handed|\n"
"#MAGICITEM|Lantern of the Long Road|Wondrous item|uncommon|requires "
"attunement|While lit, the lantern sheds bright light in a 30-foot radius "
"and dim light for another 30 feet. Creatures of your choice within the "
"bright light ignore difficult terrain caused by mud, snow or undergrowth.\n"
"#SPELL|Mendicant's Warning|1|Divination|1|0|1 action|Self|V, S|"
"1 hour|bard,cleric,warlock\n"
"#\n";

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


/* The file names a category, a school and a list of classes the way the
   data files do, so a DM editing it by hand writes "martial-melee" rather
   than a 6. A bare number is still read, so a file written by an earlier
   build still loads. */
static int all_digits(const char *s)
{
    if (!*s) return 0;
    for (; *s; s++) if (*s < '0' || *s > '9') return 0;
    return 1;
}

static int category_of(const char *s)
{
    int n = item_category_by_name(s);
    if (n >= 0) return n;
    /* A number is taken as the category index it names, but only one that
       exists: CATEGORY_LABEL is indexed by this wherever an item is
       printed, and a hand-written file naming category 999 read past the
       end of it. */
    if (all_digits(s)) {
        n = atoi(s);
        if (n >= 0 && n <= ITEM_MOUNT) return n;
    }
    return ITEM_GEAR;
}

static int school_of(const char *s)
{
    int n = school_by_name(s);
    if (n >= 0) return n;
    /* Bounded for the same reason as the category: SCHOOL_NAMES is indexed
       by it on every sheet the spell appears on. */
    if (all_digits(s)) {
        n = atoi(s);
        if (n >= 0 && n < SCHOOL_COUNT) return n;
    }
    return SCHOOL_EVOCATION;
}

static unsigned classes_of(const char *s)
{
    if (all_digits(s)) return (unsigned)atoi(s);
    return spell_classes_by_name(s);
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
            it->category = (ItemCategory)category_of(fields[2]);
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
        } else if (!strcmp(fields[0], "SPELL") && n >= 11
                   && n_spells < MAX_CUSTOM) {
            SpellData *sp = &custom_spells[n_spells++];
            sp->name = keep(fields[1]);
            sp->book = BOOK_HOMEBREW;
            {   /* Spell levels run 0 (a cantrip) to 9; a level outside
                    that belongs to no slot and no list, so it would be a
                    spell that could be written down but never cast. */
                int lv = atoi(fields[2]);
                sp->level = (unsigned char)(lv < 0 ? 0 : lv > 9 ? 9 : lv);
            }
            sp->school = (unsigned char)school_of(fields[3]);
            sp->ritual = (unsigned char)atoi(fields[4]);
            sp->concentration = (unsigned char)atoi(fields[5]);
            sp->casting_time = keep(fields[6]);
            sp->range = keep(fields[7]);
            sp->components = keep(fields[8]);
            sp->duration = keep(fields[9]);
            sp->classes = (unsigned short)classes_of(fields[10]);
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

    f = fopen(HOMEBREW_FILE, "w");
    if (!f) return -1;

    /* The explanation is written every time, even when there is nothing to
       write under it, so a DM who empties the list is not left with a blank
       file and no idea what goes in it. */
    fputs(HOMEBREW_HEADER, f);

    for (i = 0; i < n_items; i++) {
        const ItemData *it = &custom_items[i];
        fprintf(f, "ITEM|%s|%s|%d|%d|%d|%d|%d|%d|%s|%s|%s|%s\n", it->name,
                ITEM_CATEGORY_NAME[it->category], it->cost_cp,
                it->weight_tenths,
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
        char classes[160];
        spell_classes_text(sp->classes, classes, sizeof classes);
        fprintf(f, "SPELL|%s|%d|%s|%d|%d|%s|%s|%s|%s|%s\n", sp->name,
                sp->level, SCHOOL_NAMES[sp->school], sp->ritual,
                sp->concentration, sp->casting_time, sp->range,
                sp->components, sp->duration, classes);
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

/* Prices are held in copper, but nobody thinks in copper: ask for a coin
 * and an amount. */
static int ask_price(void)
{
    static const char *const coins[] = {
        "gold pieces", "silver pieces", "copper pieces", "It costs nothing"
    };
    static const int per[] = { 100, 10, 1 };
    int coin = ui_menu("  Priced in:", coins, NULL, 4);

    if (coin == 3) return 0;
    return ui_int("  How many", 0, 100000) * per[coin];
}

/* Weight is held in tenths of a pound, which is likewise not how anyone
 * describes a thing. */
static int ask_weight(void)
{
    int lb, tenths;

    lb = ui_int("  Weight in whole pounds", 0, 10000);
    tenths = ui_int("  ...and tenths of a pound", 0, 9);
    return lb * 10 + tenths;
}

static const char *const DAMAGE_TYPES[] = {
    "acid", "bludgeoning", "cold", "fire", "force", "lightning", "necrotic",
    "piercing", "poison", "psychic", "radiant", "slashing", "thunder"
};
static const char *const DAMAGE_DICE[] = {
    "1d4", "1d6", "1d8", "1d10", "1d12", "2d6", "2d8", "2d10", "1"
};

/* Armour AC reads three different ways depending on how heavy it is, and
 * "-1 means add your whole modifier" is not a question anyone can answer.
 * Ask what it does instead, and translate. */
static int ask_dex_handling(ItemCategory cat)
{
    static const char *const rules[] = {
        "Add the wearer's full Dexterity modifier (light armor)",
        "Add Dexterity, but no more than +2 (medium armor)",
        "Add no Dexterity at all (heavy armor)",
        "Add Dexterity up to some other cap"
    };
    int pick;

    if (cat == ITEM_SHIELD) return 0;   /* a shield is a flat bonus */

    pick = ui_menu("  How does Dexterity apply?", rules, NULL, 4);
    if (pick == 0) return -1;
    if (pick == 1) return 2;
    if (pick == 2) return 0;
    return ui_int("  Dexterity cap", 1, 10);
}

static int ask_strength_requirement(void)
{
    static const char *const reqs[] = {
        "None", "Strength 13", "Strength 15", "Some other score"
    };
    int pick = ui_menu("  Strength needed to wear it without being slowed:",
                       reqs, NULL, 4);

    if (pick == 0) return 0;
    if (pick == 1) return 13;
    if (pick == 2) return 15;
    return ui_int("  Strength required", 1, 30);
}

/* Weapon properties are a fixed list, and three of them carry a number, so
 * the ones that do are asked for rather than left to be typed in. */
static void ask_weapon_properties(char *out, size_t n)
{
    const char *names[16];
    int flags[16];
    int i, count = WEAPON_PROPERTY_COUNT;
    size_t used = 0;

    if (count > 16) count = 16;
    for (i = 0; i < count; i++) {
        names[i] = WEAPON_PROPERTIES[i].item;
        flags[i] = 0;
    }

    ui_toggle_list("  Properties:", names, count, flags);
    out[0] = '\0';

    /* Three of the properties print a number alongside them, so those are
       asked for once the list is settled. */
    for (i = 0; i < count; i++) {
        char detail[64];
        int w;

        if (!flags[i]) continue;
        detail[0] = '\0';

        /* Ammunition and thrown weapons print a range; a versatile weapon
           prints the die it deals in two hands. */
        if (!strcmp(names[i], "Ammunition") || !strcmp(names[i], "Thrown")
            || !strcmp(names[i], "Range")) {
            char range[32], prompt[64];
            snprintf(prompt, sizeof prompt, "    %s -- range, as normal/long",
                     names[i]);
            ui_line_default(prompt, "20/60", range, sizeof range);
            snprintf(detail, sizeof detail, " (range %s)", range);
        } else if (!strcmp(names[i], "Versatile")) {
            char die[16], prompt[64];
            snprintf(prompt, sizeof prompt,
                     "    %s -- damage held in two hands", names[i]);
            ui_line_default(prompt, "1d10", die, sizeof die);
            snprintf(detail, sizeof detail, " (%s)", die);
        }

        w = snprintf(out + used, n - used, "%s%s%s", used ? ", " : "",
                     names[i], detail);
        if (w < 0 || (size_t)w >= n - used) break;
        used += (size_t)w;
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

    it->cost_cp = ask_price();
    it->weight_tenths = ask_weight();

    if (it->category <= ITEM_SHIELD) {
        it->base_ac = ui_int(it->category == ITEM_SHIELD
                             ? "  Armor Class it adds"
                             : "  Base Armor Class", 0, 25);
        it->dex_cap = ask_dex_handling(it->category);
        it->str_req = (it->category == ITEM_SHIELD)
                    ? 0 : ask_strength_requirement();
        it->stealth_disadvantage =
            ui_yesno("  Disadvantage on Dexterity (Stealth) checks?", 0);
        it->damage = keep(""); it->damage_type = keep("");
        it->properties = keep(""); it->contents = keep("");
    } else if (it->category <= ITEM_MARTIAL_RANGED) {
        ui_pick_or_type("  Damage dice:", DAMAGE_DICE,
                        (int)(sizeof DAMAGE_DICE / sizeof DAMAGE_DICE[0]),
                        buf, sizeof buf);
        it->damage = keep(buf);

        ui_pick_or_type("  Damage type:", DAMAGE_TYPES,
                        (int)(sizeof DAMAGE_TYPES / sizeof DAMAGE_TYPES[0]),
                        buf, sizeof buf);
        it->damage_type = keep(buf);

        ask_weapon_properties(buf, sizeof buf);
        it->properties = keep(buf);
        it->contents = keep("");
    } else {
        it->damage = keep(""); it->damage_type = keep("");
        it->properties = keep("");
        if (it->category == ITEM_PACK
            || ui_yesno("  Does it hold or contain anything?", 0)) {
            ui_line("  What is in it", buf, sizeof buf);
            it->contents = keep(buf);
        } else {
            it->contents = keep("");
        }
    }

    n_items++;
    rebuild_banks();
    printf("  Added %s to the item bank.\n", it->name);
}

/* A magic item's stat line opens with its kind, often narrowed in brackets:
 * "Weapon (any sword)", "Armor (plate)". Ask for the kind, then the
 * narrowing where one makes sense. */
static const char *const MAGIC_KINDS[] = {
    "Wondrous item", "Armor", "Weapon", "Potion", "Ring", "Rod", "Scroll",
    "Staff", "Wand"
};

static void ask_magic_type(char *out, size_t n)
{
    char kind[MAX_NAME], narrowing[MAX_NAME];

    ui_pick_or_type("  What kind of magic item is it?", MAGIC_KINDS,
                    (int)(sizeof MAGIC_KINDS / sizeof MAGIC_KINDS[0]),
                    kind, sizeof kind);

    /* Only some kinds are ever narrowed; a potion or a ring never is. */
    if (!strcmp(kind, "Armor") || !strcmp(kind, "Weapon")) {
        printf("  Which sort, if it is limited to one -- \"plate\", "
               "\"any sword\", \"any axe\".\n");
        ui_line("  Leave blank for any", narrowing, sizeof narrowing);
        if (narrowing[0]) {
            snprintf(out, n, "%s (%s)", kind, narrowing);
            return;
        }
        snprintf(out, n, "%s (any)", kind);
        return;
    }
    snprintf(out, n, "%s", kind);
}

/* Attunement clauses read as a fixed set of shapes, two of which name a
 * class or an alignment. */
static const char *ask_attunement(void)
{
    static const char *const shapes[] = {
        "Anyone can attune to it",
        "requires attunement",
        "requires attunement by a spellcaster",
        "requires attunement by a class",
        "requires attunement by a creature of a given alignment"
    };
    char buf[MAX_TEXT];
    int pick = ui_menu("  Attunement:", shapes, NULL, 5);

    if (pick == 0) return NULL;
    if (pick == 1 || pick == 2) return keep(shapes[pick]);

    if (pick == 3) {
        const char *names[16];
        int i, n = CLASS_COUNT > 16 ? 16 : CLASS_COUNT;
        char lower[MAX_NAME];
        const char *chosen;

        for (i = 0; i < n; i++) names[i] = CLASSES[i].name;
        chosen = names[ui_menu("  Which class?", names, NULL, n)];

        /* The books write "by a paladin", not "by a Paladin". */
        for (i = 0; chosen[i] && i < (int)sizeof lower - 1; i++) {
            lower[i] = (chosen[i] >= 'A' && chosen[i] <= 'Z')
                     ? (char)(chosen[i] + 32) : chosen[i];
        }
        lower[i] = '\0';
        snprintf(buf, sizeof buf, "requires attunement by a %s", lower);
        return keep(buf);
    }
    {
        const char *aligns[ALIGN_COUNT];
        int i;
        for (i = 0; i < ALIGN_COUNT; i++) aligns[i] = ALIGNMENT_NAME[i];
        snprintf(buf, sizeof buf,
                 "requires attunement by a creature of %s alignment",
                 aligns[ui_menu("  Which alignment?", aligns, NULL,
                                ALIGN_COUNT)]);
        return keep(buf);
    }
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

    ask_magic_type(buf, sizeof buf);
    m->type = keep(buf);

    {
        static const char *const rarities[] = {
            "common", "uncommon", "rare", "very rare", "legendary",
            "artifact", "rarity varies"
        };
        m->rarity = keep(rarities[ui_menu("  Rarity:", rarities, NULL, 7)]);
    }

    m->attunement = ask_attunement();

    ui_line("  What does it do", buf, sizeof buf);
    m->text = keep(buf[0] ? buf : "No description was given.");

    n_magic++;
    rebuild_banks();
    printf("  Added %s to the magic item bank.\n", m->name);
}

/* The values the books actually use. Each is offered as a menu with a way
 * out, so the usual answer is one keypress and an unusual one is still
 * possible. */
static const char *const CASTING_TIMES[] = {
    "1 action", "1 bonus action", "1 reaction", "1 minute", "10 minutes",
    "1 hour", "8 hours", "12 hours", "24 hours"
};
static const char *const SPELL_RANGES[] = {
    "Self", "Touch", "5 feet", "10 feet", "30 feet", "60 feet", "90 feet",
    "120 feet", "150 feet", "300 feet", "500 feet", "1 mile", "Sight",
    "Unlimited"
};
static const char *const SPELL_DURATIONS[] = {
    "Instantaneous", "1 round", "1 minute", "10 minutes", "1 hour",
    "8 hours", "24 hours", "7 days", "Until dispelled",
    "Until dispelled or triggered", "Special"
};
static const char *const SPELL_LEVELS[] = {
    "Cantrip", "1st level", "2nd level", "3rd level", "4th level",
    "5th level", "6th level", "7th level", "8th level", "9th level"
};

/* Components read as "V, S, M (a pinch of soot)", so they are built from
 * three toggles and, if there is a material, its description. */
static void ask_components(char *out, size_t n)
{
    static const char *const parts[] = {
        "Verbal", "Somatic", "Material"
    };
    int flags[3] = { 1, 1, 0 };
    char material[MAX_TEXT / 2];
    size_t used = 0;

    ui_toggle_list("  Components:", parts, 3, flags);

    out[0] = '\0';
    if (flags[0]) used += (size_t)snprintf(out + used, n - used, "V");
    if (flags[1]) used += (size_t)snprintf(out + used, n - used, "%sS",
                                           used ? ", " : "");
    if (flags[2]) {
        used += (size_t)snprintf(out + used, n - used, "%sM",
                                 used ? ", " : "");
        ui_line("  What material does it need", material,
                sizeof material < n - used ? sizeof material : n - used);
        if (material[0]) {
            snprintf(out + used, n - used, " (%s)", material);
        }
    }
    if (!out[0]) snprintf(out, n, "None");
}

/* Which class lists a spell appears on decides who can ever learn it, so it
 * is a checklist rather than a run of yes-or-no questions. */
static int ask_spell_classes(unsigned short *classes)
{
    static const char *const names[9] = {
        "Bard", "Cleric", "Druid", "Paladin", "Ranger", "Sorcerer",
        "Warlock", "Wizard", "Artificer"
    };
    static const unsigned short bits[9] = {
        SPL_BARD, SPL_CLERIC, SPL_DRUID, SPL_PALADIN, SPL_RANGER,
        SPL_SORCERER, SPL_WARLOCK, SPL_WIZARD, SPL_ARTIFICER
    };
    int flags[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    int i, set;

    printf("\n  A spell has to be on at least one class list, or nobody "
           "could ever learn it.\n");
    set = ui_toggle_list("  Which class spell lists is it on?", names, 9,
                         flags);
    if (set == 0) return 0;

    *classes = 0;
    for (i = 0; i < 9; i++) {
        if (flags[i]) *classes |= bits[i];
    }
    return 1;
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

    sp->level = (unsigned char)ui_menu("  What level is it?", SPELL_LEVELS,
                                       NULL, 10);

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

    ui_pick_or_type("  Casting time:", CASTING_TIMES,
                    (int)(sizeof CASTING_TIMES / sizeof CASTING_TIMES[0]),
                    buf, sizeof buf);
    sp->casting_time = keep(buf);

    ui_pick_or_type("  Range:", SPELL_RANGES,
                    (int)(sizeof SPELL_RANGES / sizeof SPELL_RANGES[0]),
                    buf, sizeof buf);
    sp->range = keep(buf);

    ask_components(buf, sizeof buf);
    sp->components = keep(buf);

    ui_pick_or_type("  Duration:", SPELL_DURATIONS,
                    (int)(sizeof SPELL_DURATIONS / sizeof SPELL_DURATIONS[0]),
                    buf, sizeof buf);
    sp->duration = keep(buf);

    if (!ask_spell_classes(&sp->classes)) {
        printf("  No class list chosen, so nobody could learn it. Not "
               "added.\n");
        return;
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
