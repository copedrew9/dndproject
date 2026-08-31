/* build.c -- the character creation wizard (PHB chapter 1, steps 1-6). */
#include "build.h"
#include "saveload.h"
#include "ui.h"

#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>
#include <string.h>



/* ------------------------------------------------------------- list helpers */

int has_language(const Character *c, const char *lang)
{
    int i;
    for (i = 0; i < c->language_count; i++) {
        if (strcmp(c->languages[i], lang) == 0) return 1;
    }
    return 0;
}

void add_language(Character *c, const char *lang)
{
    if (!lang || !*lang || has_language(c, lang)) return;
    if (c->language_count >= MAX_LANGS) return;
    strncpy(c->languages[c->language_count], lang, MAX_NAME - 1);
    c->languages[c->language_count][MAX_NAME - 1] = '\0';
    c->language_count++;
}

int has_tool(const Character *c, const char *tool)
{
    int i;
    for (i = 0; i < c->tool_prof_count; i++) {
        if (strcmp(c->tool_profs[i], tool) == 0) return 1;
    }
    return 0;
}

void add_tool(Character *c, const char *tool)
{
    char norm[MAX_NAME];
    int i;

    if (!tool || !*tool) return;
    strncpy(norm, tool, sizeof norm - 1);
    norm[sizeof norm - 1] = '\0';
    if (norm[0] >= 'a' && norm[0] <= 'z') norm[0] -= 32;

    for (i = 0; i < c->tool_prof_count; i++) {
        if (strcmp(c->tool_profs[i], norm) == 0) return;
    }
    if (c->tool_prof_count >= MAX_PROFS) return;
    strcpy(c->tool_profs[c->tool_prof_count], norm);
    c->tool_prof_count++;
}


/* ------------------------------------------------- tool proficiency phrases */

/* Backgrounds and classes word tool proficiencies as prose: "Herbalism kit",
 * "One type of gaming set", "Three musical instruments of your choice".
 * A phrase that names a definite tool is simply granted; one that leaves the
 * choice open turns into a menu of that group, taken from the equipment
 * tables, with room at the bottom for something the books do not print.
 */

/* How many the phrase asks for. Only the bard's three instruments and the
   monk's one go above one, but the words are cheap to read. */
static int tool_phrase_count(const char *phrase)
{
    if (contains_ci(phrase, "three")) return 3;
    if (contains_ci(phrase, "two")) return 2;
    return 1;
}

/* Does the phrase leave the choice to the player? */
static int tool_phrase_is_choice(const char *phrase)
{
    return contains_ci(phrase, "your choice")
        || contains_ci(phrase, "one type of")
        || contains_ci(phrase, "artisan")
        || contains_ci(phrase, "gaming set")
        || contains_ci(phrase, "musical instrument");
}

/* The tools a phrase may be satisfied with. The monk's line names two
   groups at once, so the groups are gathered rather than matched one of. */
static int tool_phrase_options(const char *phrase, const char **out, int max)
{
    static const struct { const char *word; const char *group; } GROUPS[] = {
        { "artisan",           "Artisan's tools" },
        { "gaming set",        "Gaming set" },
        { "musical instrument", "Musical instrument" }
    };
    int n = 0;
    size_t g;

    for (g = 0; g < sizeof GROUPS / sizeof GROUPS[0]; g++) {
        if (!contains_ci(phrase, GROUPS[g].word)) continue;
        n += tools_in_group(GROUPS[g].group, out + n, max - n);
    }
    if (n == 0) n = tools_in_group("", out, max);   /* every tool */
    return n;
}

/* Grants one comma-separated piece of a tool proficiency line. */
static void grant_tool_phrase(Character *c, const char *phrase,
                              const char *source)
{
    const char *all[64];
    const char *opts[64];
    int total, i, want, n;

    if (!phrase || !*phrase) return;
    if (!tool_phrase_is_choice(phrase)) { add_tool(c, phrase); return; }

    total = tool_phrase_options(phrase, all, 64);
    want = tool_phrase_count(phrase);

    printf("\n  %s grants \"%s\".\n", source, phrase);
    while (want-- > 0) {
        char answer[MAX_NAME];

        n = 0;
        for (i = 0; i < total; i++) {
            if (!has_tool(c, all[i])) opts[n++] = all[i];
        }
        if (n == 0) {
            ui_line("  Name a tool you are not already proficient with",
                    answer, sizeof answer);
            if (!answer[0]) return;
            add_tool(c, answer);
            continue;
        }
        ui_menu_custom("  Which one?", opts, NULL, n,
                       "Another tool (type it in)", answer, sizeof answer);
        add_tool(c, answer);
    }
}

/* Splits a tool proficiency line on commas and grants each piece. */
static void grant_tool_line(Character *c, const char *line, const char *source)
{
    char buf[256], *cursor = buf, *piece;

    if (!line || !*line) return;
    strncpy(buf, line, sizeof buf - 1);
    buf[sizeof buf - 1] = '\0';
    while ((piece = next_csv(&cursor)) != NULL) {
        grant_tool_phrase(c, piece, source);
    }
}


int has_prof(const Character *c, const char *prof)
{
    int i;
    /* A class granting "All armor" is proficient with every armour type. */
    int armour = contains_ci(prof, "armor");

    for (i = 0; i < c->other_prof_count; i++) {
        if (contains_ci(c->other_profs[i], prof)) return 1;
        if (armour && contains_ci(c->other_profs[i], "all armor")) return 1;
    }
    return 0;
}

/* Whether the character may wear this kind of armour without penalty.
 *
 * Armour proficiency is held as text, the way the books word it -- "Light
 * armor", "All armor", "Shields (nonmetal)" -- so the question is asked of
 * has_prof(), which already looks for the phrase inside the line and knows
 * that "All armor" covers the three types. Shields are asked about by
 * themselves: the books list them separately from armour, and "All armor"
 * does not include them.
 *
 * Anything that is not armour answers yes, so a caller can ask about any
 * inventory entry without sorting them first.
 */
int armour_proficient(const Character *c, int category)
{
    switch (category) {
    case ITEM_LIGHT_ARMOR:  return has_prof(c, "light armor");
    case ITEM_MEDIUM_ARMOR: return has_prof(c, "medium armor");
    case ITEM_HEAVY_ARMOR:  return has_prof(c, "heavy armor");
    case ITEM_SHIELD:       return has_prof(c, "shield");
    default:                return 1;
    }
}

void add_prof(Character *c, const char *prof)
{
    char norm[MAX_NAME];
    int i;

    if (!prof || !*prof) return;

    /* The PHB writes these mid-sentence, so the same proficiency can arrive
       both capitalised and not. Normalise before the duplicate check, or the
       sheet ends up listing "Medium armor" twice. */
    strncpy(norm, prof, sizeof norm - 1);
    norm[sizeof norm - 1] = '\0';
    if (norm[0] >= 'a' && norm[0] <= 'z') norm[0] -= 32;

    for (i = 0; i < c->other_prof_count; i++) {
        if (strcmp(c->other_profs[i], norm) == 0) return;
    }
    if (c->other_prof_count >= MAX_PROFS) return;
    strcpy(c->other_profs[c->other_prof_count], norm);
    c->other_prof_count++;
}

/* One comma-separated piece of a class's proficiency line.
 *
 * Those lines mix three things the sheet keeps apart. A bard multiclassed
 * into reads "Light armor, one skill of your choice, one musical
 * instrument": the skill is granted by the skill picker, the instrument is a
 * tool to be chosen, and only the armour belongs where the whole line used
 * to go verbatim.
 */
static void grant_prof_piece(Character *c, const char *piece,
                             const char *source)
{
    int id;

    /* choose_class_skills() grants the skill; saying so again here would
       leave "one skill of your choice" sitting among the armour. */
    if (contains_ci(piece, "skill")) return;

    if (contains_ci(piece, "artisan") || contains_ci(piece, "gaming set")
        || contains_ci(piece, "musical instrument")) {
        grant_tool_phrase(c, piece, source);
        return;
    }

    id = find_item(piece);
    if (id >= 0 && ITEMS[id].category == ITEM_TOOL) {
        add_tool(c, ITEMS[id].name);
        return;
    }
    add_prof(c, piece);
}

void add_prof_list(Character *c, const char *csv, const char *source)
{
    char buf[512], *cursor = buf, *piece;

    if (!csv || !*csv || strcmp(csv, "None") == 0) return;
    strncpy(buf, csv, sizeof buf - 1);
    buf[sizeof buf - 1] = '\0';

    while ((piece = next_csv(&cursor)) != NULL) {
        if (*piece) grant_prof_piece(c, piece, source);
    }
}

void add_choice(Character *c, const char *label, const char *value)
{
    if (c->choice_count >= MAX_CHOICES) return;
    strncpy(c->choices[c->choice_count].label, label, MAX_NAME - 1);
    c->choices[c->choice_count].label[MAX_NAME - 1] = '\0';
    strncpy(c->choices[c->choice_count].value, value, MAX_TEXT - 1);
    c->choices[c->choice_count].value[MAX_TEXT - 1] = '\0';
    c->choice_count++;
}

/* The choices list is scanned in exactly three ways: how many entries carry
   a label, whether one of them starts with a given value, and whether one of
   them is exactly that value. The prefix form is what the two lists that
   record more than a name need -- an infusion is stored as "Replicate Magic
   Item (Bag of Holding)" and an optional feature as "Name (replaces X)". */
int count_choices(const Character *c, const char *label)
{
    int i, n = 0;
    for (i = 0; i < c->choice_count; i++) {
        if (strcmp(c->choices[i].label, label) == 0) n++;
    }
    return n;
}

int has_choice_starting(const Character *c, const char *label,
                        const char *value)
{
    int i;
    for (i = 0; i < c->choice_count; i++) {
        if (strcmp(c->choices[i].label, label) != 0) continue;
        if (strncmp(c->choices[i].value, value, strlen(value)) == 0) return 1;
    }
    return 0;
}

int has_choice_exactly(const Character *c, const char *label,
                       const char *value)
{
    int i;
    for (i = 0; i < c->choice_count; i++) {
        if (strcmp(c->choices[i].label, label) == 0
            && strcmp(c->choices[i].value, value) == 0) return 1;
    }
    return 0;
}

/* Adds to a stack that is already there, and stops at MAX_STACK. Buying
   ninety-nine of something twice over adds up, and nothing used to stop it
   adding up past what the weight of a pack can be held in. */
static void stack_up(int *held, int qty)
{
    if (qty > MAX_STACK - *held) *held = MAX_STACK;
    else *held += qty;
}

void add_item(Character *c, int item_id, int qty, int equipped)
{
    int i;
    if (item_id < 0 || item_id >= ITEM_COUNT || qty <= 0) return;

    for (i = 0; i < c->item_count; i++) {
        /* item_id indexes ITEMS here and MAGIC_ITEMS in a magic entry, so
           the two have to be told apart before the indices are compared:
           without this, buying the scale mail whose row happens to sit at
           the same index as the amulet already carried added a second
           amulet instead. */
        if (!c->inventory[i].is_magic
            && c->inventory[i].item_id == item_id
            && c->inventory[i].equipped == equipped) {
            stack_up(&c->inventory[i].quantity, qty);
            return;
        }
    }
    if (c->item_count >= MAX_ITEMS) return;
    c->inventory[c->item_count].item_id = item_id;
    c->inventory[c->item_count].quantity = qty;
    c->inventory[c->item_count].equipped = equipped;
    c->inventory[c->item_count].is_magic = 0;
    c->inventory[c->item_count].attuned = 0;
    c->inventory[c->item_count].plus = 0;
    c->item_count++;
}

/* Whether this exact item is already carried unequipped, so adding more of
   it stacks rather than needing a new slot. */
static int carrying_already(const Character *c, int item_id)
{
    int i;
    for (i = 0; i < c->item_count; i++) {
        if (!c->inventory[i].is_magic && c->inventory[i].item_id == item_id
            && !c->inventory[i].equipped) {
            return 1;
        }
    }
    return 0;
}

/* Whether a character has room for another kind of thing. add_item drops
   what will not fit and says nothing, which was harmless while a pack was
   one entry and is not now that it is a dozen. */
int inventory_has_room(const Character *c)
{
    return c->item_count < MAX_ITEMS;
}

/* Add an item, or -- if it is a pack -- what is in it.
 *
 * Every place a character acquires gear goes through this, so that a pack
 * cannot reach an inventory by a route somebody forgot. It could: the
 * starting-equipment resolver has an alias table that calls add_item and
 * returns before the unpacking below is reached, and the Priest's pack is
 * the one pack with an entry in it -- so a cleric whose equipment line
 * matched the alias carried "1 x Priest's pack" while all six of the
 * others came apart. Returns how many kinds of thing went on. */
int add_gear(Character *c, int item_id, int qty, int equipped)
{
    int parts;
    if (!equipped) {
        parts = add_pack(c, item_id, qty);
        if (parts > 0) return parts;
    }
    add_item(c, item_id, qty, equipped);
    return 1;
}

/* Taking a pack takes what is in it.
 *
 * A pack on the sheet is a word: "Explorer's pack" tells a player nothing
 * they can drop, sell, eat or count, and the rations and the bedroll are
 * the whole reason to have one. So the contents go on instead -- instead
 * of, not as well as, because a pack's weight IS the sum of its parts and
 * carrying both would count everything twice.
 *
 * Returns how many kinds of thing were added, or -1 when this is not a
 * pack, so the caller can fall back to adding it plainly. */
int add_pack(Character *c, int pack_id, int qty)
{
    int i, added = 0, dropped = 0;

    if (pack_id < 0 || pack_id >= ITEM_COUNT) return -1;
    if (ITEMS[pack_id].category != ITEM_PACK) return -1;
    if (qty <= 0) return -1;

    for (i = 0; i < PACK_ITEM_COUNT; i++) {
        int id;
        if (!same_fold(PACK_ITEMS[i].pack, ITEMS[pack_id].name)) continue;
        id = find_item(PACK_ITEMS[i].item);
        if (id < 0) continue;           /* a bank the DM has narrowed */
        if (!inventory_has_room(c)
            && !carrying_already(c, id)) {
            dropped++;
            continue;
        }
        add_item(c, id, PACK_ITEMS[i].quantity * qty, 0);
        added++;
    }
    if (dropped) {
        printf("      %d thing%s from the pack would not fit and was left "
               "behind.\n", dropped, dropped == 1 ? "" : "s");
    }
    /* A pack the data has no contents for stays a pack rather than
       vanishing. */
    return added ? added : -1;
}

void add_magic_item(Character *c, int magic_id, int qty, int attuned, int plus)
{
    int i;
    if (magic_id < 0 || magic_id >= MAGIC_ITEM_COUNT || qty <= 0) return;

    for (i = 0; i < c->item_count; i++) {
        if (c->inventory[i].is_magic && c->inventory[i].item_id == magic_id
            && c->inventory[i].attuned == attuned
            && c->inventory[i].plus == plus) {
            stack_up(&c->inventory[i].quantity, qty);
            return;
        }
    }
    add_magic_item_copy(c, magic_id, qty, attuned, plus);
}

/* One copy of a magic item, never merged into an existing stack.
 *
 * add_magic_item() folds a second copy into the first when the item, the
 * attunement and the bonus all match, which is what a player picking the
 * same ring up twice wants. A saved file is the other case: it already
 * says how many copies there are, and two records of one item are two
 * entries on purpose -- they differ in something the stack test does not
 * look at, the damage type the copy was made against or what the table has
 * chosen to tell the player about it. Merging those loses the second one's
 * presentation, and with it its attunement, because the merged entry has
 * only one.
 *
 * Returns the entry so a caller can fill in what it alone knows, or NULL
 * when the inventory is full.
 */
InventoryEntry *add_magic_item_copy(Character *c, int magic_id, int qty,
                                    int attuned, int plus)
{
    InventoryEntry *e;

    if (magic_id < 0 || magic_id >= MAGIC_ITEM_COUNT || qty <= 0) return NULL;
    if (c->item_count >= MAX_ITEMS) return NULL;

    e = &c->inventory[c->item_count];
    /* Slots are reused after something is put down, so everything is set
       rather than only the fields this call was given: a stale variant or
       a stale hidden flag left by the last occupant would otherwise attach
       itself to the new item. */
    memset(e, 0, sizeof *e);
    e->item_id = magic_id;
    e->quantity = qty;
    e->is_magic = 1;
    e->attuned = attuned;
    e->plus = plus;
    c->item_count++;
    return e;
}

/* Drops qty of one entry, closing the gap when nothing is left. */
void remove_inventory_entry(Character *c, int index, int qty)
{
    int i;
    if (index < 0 || index >= c->item_count || qty <= 0) return;

    c->inventory[index].quantity -= qty;
    if (c->inventory[index].quantity > 0) return;

    for (i = index; i < c->item_count - 1; i++) {
        c->inventory[i] = c->inventory[i + 1];
    }
    c->item_count--;
}

int attuned_count(const Character *c)
{
    int i, n = 0;
    for (i = 0; i < c->item_count; i++) {
        if (c->inventory[i].is_magic && c->inventory[i].attuned) n++;
    }
    return n;
}

void add_item_by_name(Character *c, const char *name, int qty, int equipped)
{
    add_item(c, find_item(name), qty, equipped);
}

/* ------------------------------------------------------- what a choice means
 *
 * Typing "3 info" at a menu asks what entry 3 would give you. What a
 * player needs at that moment is the part they cannot see from the name:
 * the numbers it moves and the proficiencies it grants. So each panel
 * leads with those and leaves the prose to the sheet, which prints it all
 * once the choice is made.
 */
static void add_bit(char *out, size_t n, const char *fmt, ...)
{
    va_list ap;
    size_t used = strlen(out);

    if (used + 2 >= n) return;
    if (used) {
        out[used++] = ' ';
        out[used] = '\0';
    }
    va_start(ap, fmt);
    vsnprintf(out + used, n - used, fmt, ap);
    va_end(ap);
}

/* "+2 Strength, +1 Constitution", or nothing when the race grants none. */
static void ability_line(const int *ability, char *out, size_t n)
{
    int a, wrote = 0;

    out[0] = '\0';
    for (a = 0; a < ABL_COUNT; a++) {
        char bit[48];
        if (!ability[a]) continue;
        snprintf(bit, sizeof bit, "%s%+d %s", wrote ? ", " : "",
                 ability[a], ABILITY_NAME[a]);
        strncat(out, bit, n - strlen(out) - 1);
        wrote = 1;
    }
}

static void race_info(const RaceData *r, char *out, size_t n)
{
    char line[160];

    out[0] = '\0';
    /* Two settings and one race flag all discard the printed increases, so
       the panel has to say which rule is actually in force rather than
       quoting a spread that will never be applied. */
    if (r->origin_choice) {
        add_bit(out, n, "Ability scores: no fixed increases -- you choose "
                        "where they go.");
    } else if (SETTINGS.custom_origins) {
        add_bit(out, n, "Ability scores: custom origins is on, so you place "
                        "this race's increases yourself.");
    } else {
        ability_line(r->ability, line, sizeof line);
        if (line[0]) add_bit(out, n, "Ability scores: %s.", line);
    }
    add_bit(out, n, "Speed %d feet, %s.", r->speed, SIZE_NAME[r->size]);
    if (r->darkvision) add_bit(out, n, "Darkvision %d feet.", r->darkvision);
    add_bit(out, n, "Languages: %s%s.", r->languages,
            r->extra_languages ? ", and one of your choice" : "");
    if (r->extra_skills) {
        add_bit(out, n, "Skills: %d of your choice.", r->extra_skills);
    }
    if (r->bonus_feats) add_bit(out, n, "A feat at 1st level.");
    if (r->subrace_count) {
        add_bit(out, n, "Has %d subraces, chosen next.", r->subrace_count);
    }
}

static void class_info(const ClassData *cl, char *out, size_t n)
{
    out[0] = '\0';
    add_bit(out, n, "Hit die d%d.", cl->hit_die);
    add_bit(out, n, "Saving throws: %s and %s.",
            ABILITY_NAME[cl->save_prof[0]], ABILITY_NAME[cl->save_prof[1]]);
    /* The books write "None" in these columns rather than leaving them
       blank, so printing them unconditionally would say "Armour: None." */
    if (strcmp(cl->armour_profs, "None")) {
        add_bit(out, n, "Armour: %s.", cl->armour_profs);
    }
    add_bit(out, n, "Weapons: %s.", cl->weapon_profs);
    if (cl->tool_profs[0] && strcmp(cl->tool_profs, "None")) {
        add_bit(out, n, "Tools: %s.", cl->tool_profs);
    }
    {
        char skills[240];
        int i, wrote = 0;
        skills[0] = '\0';
        for (i = 0; i < cl->skill_option_count; i++) {
            strncat(skills, wrote ? ", " : "",
                    sizeof skills - strlen(skills) - 1);
            strncat(skills, SKILL_NAME[cl->skill_options[i]],
                    sizeof skills - strlen(skills) - 1);
            wrote = 1;
        }
        /* The bard and the rogue may choose from every skill there is, and
           listing all eighteen crowds out what the class actually does. */
        if (cl->skill_option_count >= SKL_COUNT) {
            add_bit(out, n, "Skills: any %d.", cl->skill_picks);
        } else if (wrote) {
            add_bit(out, n, "Skills: %d from %s.", cl->skill_picks, skills);
        }
    }
    if (cl->caster != CAST_NONE) {
        add_bit(out, n, "Casts with %s, from level %d.",
                ABILITY_NAME[cl->spell_ability], cl->caster_start_level);
    }
    add_bit(out, n, "%s is chosen at level %d.",
            cl->subclass_label, cl->subclass_level);
    /* What the class actually does on the day you take it. */
    {
        int i, wrote = 0;
        char feats[320];
        feats[0] = '\0';
        for (i = 0; i < FEATURE_COUNT; i++) {
            const FeatureData *f = &FEATURES[i];
            if (f->class_id != (int)(cl - CLASSES)) continue;
            if (f->subclass_id >= 0 || f->level != 1) continue;
            strncat(feats, wrote ? ", " : "",
                    sizeof feats - strlen(feats) - 1);
            strncat(feats, f->name, sizeof feats - strlen(feats) - 1);
            wrote = 1;
        }
        if (wrote) add_bit(out, n, "At 1st level: %s.", feats);
    }
}

static void background_info(const BackgroundData *b, char *out, size_t n)
{
    int i, wrote = 0;
    char skills[200];

    out[0] = '\0';
    skills[0] = '\0';
    for (i = 0; i < 2; i++) {
        /* SKL_COUNT marks a skill the book leaves to you; naming it would
           index one past the end of SKILL_NAME. */
        if (b->skills[i] == SKL_COUNT) continue;
        strncat(skills, wrote ? ", " : "", sizeof skills - strlen(skills) - 1);
        strncat(skills, SKILL_NAME[b->skills[i]],
                sizeof skills - strlen(skills) - 1);
        wrote = 1;
    }
    if (wrote) add_bit(out, n, "Skills: %s.", skills);
    if (b->skill_choice_count) {
        add_bit(out, n, "And %d more from: %s.", b->skill_choice_count,
                b->skill_choices);
    }
    if (b->tool_profs[0]) add_bit(out, n, "Tools: %s.", b->tool_profs);
    if (b->extra_languages) {
        add_bit(out, n, "Languages: %d of your choice.", b->extra_languages);
    }
    add_bit(out, n, "Feature -- %s: %s", b->feature_name, b->feature_summary);
    if (b->gold) add_bit(out, n, "Starting gold %d gp.", b->gold);
}

/* ------------------------------------------------------------------- step 1 */

static void choose_race(Character *c)
{
    const char *names[MENU_MAX];
    const char *details[MENU_MAX];
    int i, pick;

    ui_header("Step 1: Choose a Race");
    ui_para("Your race establishes your general appearance and the natural "
            "talents you gain from ancestry, and increases one or more "
            "ability scores.");

    {
        int map[MENU_MAX], n = 0;
        static char panels[MENU_MAX][1024];
        const char *info[MENU_MAX];
        for (i = 0; i < RACE_COUNT && n < MENU_MAX; i++) {
            if (!book_enabled(RACES[i].book)) continue;
            names[n] = RACES[i].name;
            details[n] = NULL;
            race_info(&RACES[i], panels[n], sizeof panels[n]);
            info[n] = panels[n];
            map[n] = i;
            n++;
        }
        pick = map[ui_menu_info("Races:", names, details, n, info)];
    }
    c->race_id = pick;

    printf("\n%s traits:\n", RACES[pick].name);
    {
        char buf[2048];
        const char *parts[16];
        int n = split_pipe(RACES[pick].traits, buf, sizeof buf, parts, 16);
        for (i = 0; i < n; i++) {
            printf("  - ");
            ui_wrap(parts[i], 4);
        }
    }

    /* Subrace. */
    c->subrace_id = -1;
    if (RACES[pick].subrace_count > 0) {
        int first = RACES[pick].first_subrace;
        int total = RACES[pick].subrace_count;
        int map[MENU_MAX], n = 0;
        for (i = 0; i < total && n < MENU_MAX; i++) {
            if (!book_enabled(SUBRACES[first + i].book)) continue;
            names[n] = SUBRACES[first + i].name;
            details[n] = SUBRACES[first + i].traits;
            map[n] = first + i;
            n++;
        }
        if (n > 0) c->subrace_id = map[ui_menu("Subraces:", names, details, n)];
    }

    /* Draconic ancestry. */
    c->ancestry_id = -1;
    if (RACES[pick].has_ancestry) {
        char labels[16][64];
        for (i = 0; i < ANCESTRY_COUNT; i++) {
            snprintf(labels[i], sizeof labels[i], "%s -- %s, %s",
                     ANCESTRIES[i].dragon, ANCESTRIES[i].damage,
                     ANCESTRIES[i].breath);
            names[i] = labels[i];
            details[i] = NULL;
        }
        c->ancestry_id = ui_menu("Draconic ancestry:", names, details,
                                 ANCESTRY_COUNT);
    }

    /* Languages granted outright. */
    {
        char buf[256], *cursor = buf, *piece;
        strncpy(buf, RACES[pick].languages, sizeof buf - 1);
        buf[sizeof buf - 1] = '\0';
        while ((piece = next_csv(&cursor)) != NULL) add_language(c, piece);
    }

    /* The skills a race's traits promise.
     *
     * These were prose and nothing else for every race but the elf and the
     * half-orc, which were the two written out by hand here. Fourteen more
     * printed "proficiency in Perception" on the sheet and left Perception
     * unproficient, so the modifier and the passive Perception were both
     * short. They come off the row now, so a race added to data/ brings its
     * skills with it. */
    {
        char buf[256], *cursor = buf, *piece;
        snprintf(buf, sizeof buf, "%s", RACES[pick].fixed_skills);
        while ((piece = next_csv(&cursor)) != NULL) {
            int sk;
            if (!*piece) continue;
            sk = skill_by_name(piece);
            if (sk >= 0) c->skill_prof[sk] = 1;
        }
    }
    /* And the ones it offers a choice of: "proficiency in two of Animal
       Handling, Medicine, Nature, Perception, Stealth or Survival". */
    if (RACES[pick].choice_skill_count > 0
        && RACES[pick].choice_skills[0]) {
        const char *opts[SKL_COUNT];
        static char names[SKL_COUNT][MAX_NAME];
        int map[SKL_COUNT], n = 0, picks[SKL_COUNT], got, k;
        char buf[256], *cursor = buf, *piece;

        snprintf(buf, sizeof buf, "%s", RACES[pick].choice_skills);
        while ((piece = next_csv(&cursor)) != NULL && n < SKL_COUNT) {
            int sk;
            if (!*piece) continue;
            sk = skill_by_name(piece);
            if (sk < 0 || c->skill_prof[sk]) continue;
            snprintf(names[n], sizeof names[n], "%s", SKILL_NAME[sk]);
            opts[n] = names[n];
            map[n] = sk;
            n++;
        }
        if (n > 0) {
            char prompt[96];
            int want = RACES[pick].choice_skill_count;
            if (want > n) want = n;
            snprintf(prompt, sizeof prompt,
                     "%s: choose %d skill%s:", RACES[pick].name, want,
                     want == 1 ? "" : "s");
            got = ui_multi(prompt, opts, NULL, n, want, picks);
            for (k = 0; k < got; k++) {
                if (picks[k] >= 0) c->skill_prof[map[picks[k]]] = 1;
            }
        }
    }
    if (c->subrace_id >= 0) {
        const char *sn = SUBRACES[c->subrace_id].name;
        if (strcmp(sn, "Mountain Dwarf") == 0) {
            add_prof(c, "Light armor");
            add_prof(c, "Medium armor");
        } else if (strcmp(sn, "High Elf") == 0
                   || strcmp(sn, "Wood Elf") == 0) {
            add_prof(c, "Longsword");
            add_prof(c, "Shortsword");
            add_prof(c, "Shortbow");
            add_prof(c, "Longbow");
        } else if (strcmp(sn, "Dark Elf (Drow)") == 0) {
            add_prof(c, "Rapier");
            add_prof(c, "Shortsword");
            add_prof(c, "Hand crossbow");
        } else if (strcmp(sn, "Rock Gnome") == 0) {
            add_tool(c, "Tinker's tools");
        } else if (strcmp(sn, "Elf Weapon Training") == 0) {
            /* The half-elf variant of the same name, taken in place of
               Skill Versatility. */
            add_prof(c, "Longsword");
            add_prof(c, "Shortsword");
            add_prof(c, "Shortbow");
            add_prof(c, "Longbow");
        } else if (strcmp(sn, "Keen Senses") == 0) {
            c->skill_prof[SKL_PERCEPTION] = 1;
        }
    }
    if (strcmp(RACES[pick].name, "Dwarf") == 0) {
        static const char *const tools[] = {
            "Smith's tools", "Brewer's supplies", "Mason's tools" };
        int t = ui_menu("Dwarven tool proficiency:", tools, NULL, 3);
        add_tool(c, tools[t]);
        add_prof(c, "Battleaxe");
        add_prof(c, "Handaxe");
        add_prof(c, "Light hammer");
        add_prof(c, "Warhammer");
    }
}

/* ------------------------------------------------------------------- step 2 */

int multiclass_ok_public(const Character *c, int class_id, int *why)
{
    const ClassData *cd = &CLASSES[class_id];
    int i, met = 0;

    for (i = 0; i < cd->mc_req_count; i++) {
        if (ability_score(c, cd->mc_req[i]) >= cd->mc_req_score[i]) met++;
    }
    *why = 0;
    if (cd->mc_req_either) return met >= 1;
    return met == cd->mc_req_count;
}

static void describe_mc_req(int class_id, char *out, size_t n)
{
    const ClassData *cd = &CLASSES[class_id];
    if (cd->mc_req_count == 1) {
        snprintf(out, n, "%s %d", ABILITY_NAME[cd->mc_req[0]],
                 cd->mc_req_score[0]);
    } else {
        snprintf(out, n, "%s %d %s %s %d",
                 ABILITY_NAME[cd->mc_req[0]], cd->mc_req_score[0],
                 cd->mc_req_either ? "or" : "and",
                 ABILITY_NAME[cd->mc_req[1]], cd->mc_req_score[1]);
    }
}

static void choose_classes(Character *c, int *target_level)
{
    const char *names[MENU_MAX];
    const char *details[MENU_MAX];
    static char class_panels[MENU_MAX][1024];
    const char *class_info_list[MENU_MAX];
    int class_map[MENU_MAX], class_n;
    int i, remaining;

    ui_header("Step 2: Choose a Class");
    ui_para("Your class describes your vocation, your special talents and the "
            "tactics you are most likely to employ.");

    *target_level = ui_int("\nWhat total character level are you building to",
                           1, MAX_LEVEL);

    {
        int n = 0;
        for (i = 0; i < CLASS_COUNT && n < MENU_MAX; i++) {
            if (!book_enabled(CLASSES[i].book)) continue;
            names[n] = CLASSES[i].name;
            details[n] = CLASSES[i].quick_build;
            class_map[n] = i;
            n++;
        }
        class_n = n;
        for (i = 0; i < class_n; i++) {
            class_info(&CLASSES[class_map[i]], class_panels[i],
                       sizeof class_panels[i]);
            class_info_list[i] = class_panels[i];
        }
    }

    c->class_count = 0;
    remaining = *target_level;

    while (remaining > 0) {
        int pick;
        int slot;

        /* Without the multiclassing rule the whole build is one class. */
        if (!SETTINGS.multiclassing && c->class_count == 1) {
            c->classes[0].level += remaining;
            remaining = 0;
            break;
        }

        pick = class_map[ui_menu_info(c->class_count == 0
                                          ? "Classes:"
                                          : "Add levels in which class?",
                                      names, details, class_n,
                                      class_info_list)];
        slot = find_class_slot(c, pick);
        int levels, maxlev;

        if (slot < 0) {
            if (c->class_count >= MAX_CLASSES) {
                printf("  You already have as many classes as this program "
                       "tracks.\n");
                continue;
            }
            slot = c->class_count++;
            c->classes[slot].class_id = pick;
            c->classes[slot].level = 0;
            c->classes[slot].subclass_id = -1;
            c->classes[slot].subclass_option = -1;
        }

        maxlev = remaining;
        if (c->classes[slot].level + maxlev > MAX_LEVEL) {
            maxlev = MAX_LEVEL - c->classes[slot].level;
        }
        if (maxlev < 1) {
            printf("  No levels left to assign there.\n");
            continue;
        }

        if (remaining == 1) {
            levels = 1;
        } else {
            char prompt[128];
            snprintf(prompt, sizeof prompt,
                     "  How many levels in %s (%d left to assign)",
                     CLASSES[pick].name, remaining);
            levels = ui_int(prompt, 1, maxlev);
        }
        c->classes[slot].level += levels;
        remaining -= levels;

        if (remaining > 0) {
            printf("\n  %d level%s still to assign.\n", remaining,
                   remaining == 1 ? "" : "s");
        }
    }

    printf("\nYou are building: ");
    for (i = 0; i < c->class_count; i++) {
        printf("%s%s %d", i ? " / " : "",
               CLASSES[c->classes[i].class_id].name, c->classes[i].level);
    }
    printf("\n");
}

/* Grants the proficiencies each class contributes: the full set for the
 * class you started in, the reduced multiclass set for any others. */
static void grant_class_proficiencies(Character *c)
{
    int i;
    for (i = 0; i < c->class_count; i++) {
        const ClassData *cd = &CLASSES[c->classes[i].class_id];
        if (i == 0) {
            add_prof_list(c, cd->armour_profs, cd->name);
            add_prof_list(c, cd->weapon_profs, cd->name);
            c->save_prof[cd->save_prof[0]] = 1;
            c->save_prof[cd->save_prof[1]] = 1;
        } else {
            add_prof_list(c, cd->mc_profs, cd->name);
        }
    }
}

/* ------------------------------------------------------------------- step 3 */

static const int POINT_COST[16] = {
    /* index by score; only 8..15 are legal */
    0,0,0,0,0,0,0,0, 0,1,2,3,4,5,7,9
};

static void assign_pool(Character *c, int *pool, int n)
{
    int used[8];
    int a;

    memset(used, 0, sizeof used);
    printf("\nAssign these scores to your abilities:\n  ");
    for (a = 0; a < n; a++) printf("%d ", pool[a]);
    printf("\n");

    for (a = 0; a < ABL_COUNT; a++) {
        char labels[8][32];
        const char *opts[8];
        int map[8], count = 0, i, pick;

        for (i = 0; i < n; i++) {
            if (used[i]) continue;
            snprintf(labels[count], sizeof labels[count], "%d", pool[i]);
            opts[count] = labels[count];
            map[count] = i;
            count++;
        }
        {
            char prompt[64];
            snprintf(prompt, sizeof prompt, "Score for %s:", ABILITY_NAME[a]);
            pick = ui_menu(prompt, opts, NULL, count);
        }
        used[map[pick]] = 1;
        c->base_score[a] = pool[map[pick]];
    }
}

static void point_buy(Character *c)
{
    int points = 27;
    int a;

    for (a = 0; a < ABL_COUNT; a++) c->base_score[a] = 8;

    ui_para("You have 27 points. Scores run from 8 to 15 before racial "
            "increases. Costs: 8=0, 9=1, 10=2, 11=3, 12=4, 13=5, 14=7, 15=9.");

    for (a = 0; a < ABL_COUNT; a++) {
        int lo = 8, hi = 15;
        char prompt[128];

        /* Nothing is reserved for the abilities still to come: every one of
           them may stay at 8, so the whole purse is available here. */
        while (hi > 8 && POINT_COST[hi] > points) hi--;

        snprintf(prompt, sizeof prompt, "%s (%d point%s left)",
                 ABILITY_NAME[a], points, points == 1 ? "" : "s");
        c->base_score[a] = ui_int(prompt, lo, hi);
        points -= POINT_COST[c->base_score[a]];
    }

    if (points > 0) {
        printf("\n  You have %d unspent point%s.\n", points,
               points == 1 ? "" : "s");
    }
}

static void choose_abilities(Character *c)
{
    static const char *const methods[] = {
        "Standard array (15, 14, 13, 12, 10, 8)",
        "Roll 4d6 and drop the lowest, six times",
        "Point buy (27 points)",
        "Enter six scores manually"
    };
    int pool[6];
    int m, i;

    ui_header("Step 3: Determine Ability Scores");
    ui_para("Much of what your character does depends on the six abilities: "
            "Strength, Dexterity, Constitution, Intelligence, Wisdom and "
            "Charisma.");

    m = ui_menu("How would you like to generate them?", methods, NULL, 4);

    switch (m) {
    case 0: {
        static const int arr[6] = {15, 14, 13, 12, 10, 8};
        memcpy(pool, arr, sizeof arr);
        assign_pool(c, pool, 6);
        break;
    }
    case 1:
        for (;;) {
            printf("\n  Rolled: ");
            for (i = 0; i < 6; i++) {
                pool[i] = ui_roll_4d6_drop_lowest("an ability score");
                printf("%d ", pool[i]);
            }
            printf("\n");
            if (!ui_yesno("  Reroll the whole set?", 0)) break;
        }
        assign_pool(c, pool, 6);
        break;
    case 2:
        point_buy(c);
        break;
    default:
        ui_para("Enter each score as your DM gave it to you.");
        for (i = 0; i < ABL_COUNT; i++) {
            c->base_score[i] = ui_int(ABILITY_NAME[i], 1, 20);
        }
        break;
    }
}

/* Applies racial increases, including the ones the player chooses.
 *
 * With Tasha's Customizing Your Origin switched on, the fixed increases a
 * race grants become a pool the player may place anywhere, the granted
 * languages may be swapped, and a fixed racial proficiency may be traded.
 */
static void custom_origin_abilities(Character *c, int total,
                                    const char *why)
{
    static const char *const modes[] = {
        "+2 to one ability and +1 to another",
        "+1 to each of three abilities"
    };
    const char *opts[ABL_COUNT];
    int avail[ABL_COUNT], picks[3], a, m;

    for (a = 0; a < ABL_COUNT; a++) {
        opts[a] = ABILITY_NAME[a];
        avail[a] = 1;
    }

    printf("\n  %s: %d points of ability increase to place as you "
           "like.\n", why, total);

    if (total == 3) {
        m = ui_menu("  How would you like to spread them?", modes, NULL, 2);
        if (m == 0) {
            ui_multi("  +2 to which ability?", opts, avail, ABL_COUNT, 1, picks);
            if (picks[0] >= 0) {
                c->racial_bonus[picks[0]] += 2;
                avail[picks[0]] = 0;
            }
            ui_multi("  +1 to which ability?", opts, avail, ABL_COUNT, 1, picks);
            if (picks[0] >= 0) c->racial_bonus[picks[0]] += 1;
        } else {
            ui_multi("  +1 to which three abilities?", opts, avail,
                     ABL_COUNT, 3, picks);
            for (a = 0; a < 3; a++) {
                if (picks[a] >= 0) c->racial_bonus[picks[a]] += 1;
            }
        }
        return;
    }

    /* Any other total is spread one point at a time. */
    for (a = 0; a < total; a++) {
        char prompt[64];
        snprintf(prompt, sizeof prompt, "  Place +1 (%d of %d):", a + 1, total);
        ui_multi(prompt, opts, NULL, ABL_COUNT, 1, picks);
        if (picks[0] >= 0) c->racial_bonus[picks[0]] += 1;
    }
}

/* The languages a character could still learn: the books in play, minus the
 * ones already spoken. The list is filtered here rather than at each of the
 * four places that asks for a language, because SCAG's regional tongues take
 * it well past the sixteen the PHB prints -- past what those places used to
 * size their arrays for. */
static int language_choices(const Character *c, const char **opts, int *avail,
                            int max)
{
    int k, n = 0;
    for (k = 0; k < LANGUAGE_COUNT && n < max; k++) {
        if (!book_enabled(LANGUAGES[k].book)) continue;
        opts[n] = LANGUAGES[k].name;
        avail[n] = !has_language(c, LANGUAGES[k].name);
        n++;
    }
    return n;
}

static void custom_origin_languages(Character *c)
{
    ui_para("Customizing Your Origin also lets you swap the languages your "
            "race grants. Common is always kept.");
    if (!ui_yesno("  Replace your racial languages?", 0)) return;

    /* Keep Common, drop the rest, then choose replacements. */
    {
        int kept = 0, i, dropped;
        char keep[MAX_LANGS][MAX_NAME];
        for (i = 0; i < c->language_count; i++) {
            if (strcmp(c->languages[i], "Common") == 0) {
                strcpy(keep[kept++], c->languages[i]);
            }
        }
        dropped = c->language_count - kept;
        c->language_count = kept;
        for (i = 0; i < kept; i++) strcpy(c->languages[i], keep[i]);

        for (i = 0; i < dropped; i++) {
            const char *opts[MENU_MAX];
            int avail[MENU_MAX], picks[1], n;
            n = language_choices(c, opts, avail, MENU_MAX);
            ui_multi("  Replacement language:", opts, avail, n, 1, picks);
            if (picks[0] >= 0) add_language(c, opts[picks[0]]);
        }
    }
}

static void apply_racial_bonuses(Character *c)
{
    const RaceData *r = &RACES[c->race_id];
    const SubraceData *s = (c->subrace_id >= 0) ? &SUBRACES[c->subrace_id] : NULL;
    int choice_count = r->choice_asi_count;
    int exclude_cha = 0;
    int a;

    memset(c->racial_bonus, 0, sizeof c->racial_bonus);

    /* Monsters of the Multiverse races carry no fixed increases by design,
       so they use the choose-your-own spread even under the PHB rules. */
    if (r->origin_choice) {
        custom_origin_abilities(c, 3, "This race has no fixed ability "
                                "increases");
        add_choice(c, "Origin", "Chosen (no fixed increases)");
    } else if (SETTINGS.custom_origins) {
        /* Total the points the race and subrace would have granted, then let
           the player place them wherever they like. */
        int total = 0;
        if (!(s && s->replaces_race_asi)) {
            for (a = 0; a < ABL_COUNT; a++) total += r->ability[a];
        }
        if (s) for (a = 0; a < ABL_COUNT; a++) total += s->ability[a];
        total += r->choice_asi_count * (r->choice_asi_amount ? r->choice_asi_amount : 1);
        if (s && s->choice_asi_count) total += s->choice_asi_count;
        if (total < 1) total = 3;

        custom_origin_abilities(c, total, "Customizing Your Origin");
        add_choice(c, "Origin", "Customized (Tasha's)");
    } else {
        if (!(s && s->replaces_race_asi)) {
            for (a = 0; a < ABL_COUNT; a++) c->racial_bonus[a] += r->ability[a];
        }
        if (s) {
            for (a = 0; a < ABL_COUNT; a++) c->racial_bonus[a] += s->ability[a];
            if (s->choice_asi_count) choice_count = s->choice_asi_count;
        }

        /* The half-elf's two free points may not go into Charisma. */
        if (strcmp(r->name, "Half-Elf") == 0) exclude_cha = 1;

        if (choice_count > 0) {
            const char *opts[ABL_COUNT];
            int avail[ABL_COUNT], picks[4], i, n = 0;

            for (a = 0; a < ABL_COUNT; a++) {
                opts[n] = ABILITY_NAME[a];
                avail[n] = !(exclude_cha && a == ABL_CHA);
                n++;
            }
            ui_multi("Racial ability increase: +1 to which abilities?",
                     opts, avail, n, choice_count, picks);
            for (i = 0; i < choice_count; i++) {
                if (picks[i] >= 0) c->racial_bonus[picks[i]] += 1;
            }
        }
    }

    /* Extra languages from race and subrace. */
    {
        int extra = r->extra_languages + (s ? s->extra_languages : 0);
        for (a = 0; a < extra; a++) {
            const char *opts[MENU_MAX];
            int avail[MENU_MAX], picks[1], n;
            n = language_choices(c, opts, avail, MENU_MAX);
            ui_multi("Extra language from your race:", opts, avail, n,
                     1, picks);
            if (picks[0] >= 0) add_language(c, opts[picks[0]]);
        }
    }

    if (SETTINGS.custom_origins) custom_origin_languages(c);

    /* Half-elf skill versatility and the variant human's free skill. */
    {
        int extra = r->extra_skills + (s ? s->extra_skills : 0);
        if (extra > 0) {
            const char *opts[SKL_COUNT];
            int avail[SKL_COUNT], picks[4], i;
            for (i = 0; i < SKL_COUNT; i++) {
                opts[i] = SKILL_NAME[i];
                avail[i] = !c->skill_prof[i];
            }
            ui_multi("Racial skill proficiency:", opts, avail, SKL_COUNT,
                     extra, picks);
            for (i = 0; i < extra; i++) {
                if (picks[i] >= 0) c->skill_prof[picks[i]] = 1;
            }
        }
    }
}

static void check_multiclass_requirements(Character *c)
{
    int i, bad = 0;

    if (c->class_count < 2) return;

    for (i = 0; i < c->class_count; i++) {
        int why;
        if (!multiclass_ok_public(c, c->classes[i].class_id, &why)) {
            char req[128];
            describe_mc_req(c->classes[i].class_id, req, sizeof req);
            if (!bad) {
                printf("\n");
                ui_rule();
                printf("  Multiclassing prerequisites not met\n");
                ui_rule();
            }
            printf("  %s requires %s; you have ",
                   CLASSES[c->classes[i].class_id].name, req);
            {
                const ClassData *cd = &CLASSES[c->classes[i].class_id];
                int k;
                for (k = 0; k < cd->mc_req_count; k++) {
                    printf("%s%s %d", k ? ", " : "",
                           ABILITY_ABBREV[cd->mc_req[k]],
                           ability_score(c, cd->mc_req[k]));
                }
            }
            printf(".\n");
            bad = 1;
        }
    }

    if (bad) {
        ui_para("The PHB requires these minimums to take levels in more than "
                "one class. The character is recorded as built; check with "
                "your DM, or start again and adjust your scores.");
    }
}

/* ------------------------------------------------------------------- step 4 */

/* The PHB's own rules for building a background instead of taking one:
 * choose any two skills, a total of two tools or languages, keep or replace
 * the feature, and pick the equipment or take coin for it. */
static void custom_background(Character *c)
{
    char buf[MAX_NAME];
    int picks[2], i;

    ui_header("A Background of Your Own");
    ui_para("The Player's Handbook lets you build a background rather than "
            "take one: any two skills, two tools or languages between them, "
            "and any feature you like -- one from another background, or one "
            "you write yourself.");

    ui_line_default("  What is it called", "Custom Background", buf,
                    sizeof c->background_name);
    snprintf(c->background_name, sizeof c->background_name, "%s", buf);
    c->background_id = -1;

    /* Any two skills. */
    {
        const char *opts[SKL_COUNT];
        int avail[SKL_COUNT];
        for (i = 0; i < SKL_COUNT; i++) {
            opts[i] = SKILL_NAME[i];
            avail[i] = !c->skill_prof[i];
        }
        ui_multi("  Which two skills?", opts, avail, SKL_COUNT, 2, picks);
        for (i = 0; i < 2; i++) {
            if (picks[i] >= 0) c->skill_prof[picks[i]] = 1;
        }
    }

    /* Two tools or languages, in any mix. */
    for (i = 0; i < 2; i++) {
        static const char *const kinds[] = {
            "A tool proficiency", "A language"
        };
        char prompt[64];

        snprintf(prompt, sizeof prompt, "  Choice %d of 2:", i + 1);
        if (ui_menu(prompt, kinds, NULL, 2) == 0) {
            ui_line("    Which tool", buf, sizeof buf);
            if (buf[0]) add_tool(c, buf);
        } else {
            const char *opts[MENU_MAX];
            int avail[MENU_MAX], lang[1], n;
            n = language_choices(c, opts, avail, MENU_MAX);
            ui_multi("    Which language?", opts, avail, n, 1, lang);
            if (lang[0] >= 0) add_language(c, opts[lang[0]]);
        }
    }

    /* A feature: one from the printed backgrounds, or your own. */
    {
        const char *opts[32];
        const char *det[32];
        int map[32], n = 0, pick;

        for (i = 0; i < BACKGROUND_COUNT && n < 31; i++) {
            if (!book_enabled(BACKGROUNDS[i].book)) continue;
            opts[n] = BACKGROUNDS[i].feature_name;
            det[n] = BACKGROUNDS[i].feature_summary;
            map[n] = i;
            n++;
        }
        opts[n] = "Write my own";
        det[n] = NULL;

        pick = ui_menu("  Which feature?", opts, det, n + 1);
        if (pick == n) {
            ui_line("    What is the feature called", c->background_feature,
                    sizeof c->background_feature);
            ui_line("    What does it do", c->background_feature_text,
                    sizeof c->background_feature_text);
        } else {
            snprintf(c->background_feature, sizeof c->background_feature,
                     "%s", BACKGROUNDS[map[pick]].feature_name);
            snprintf(c->background_feature_text,
                     sizeof c->background_feature_text, "%s",
                     BACKGROUNDS[map[pick]].feature_summary);
        }
        if (!c->background_feature[0]) {
            snprintf(c->background_feature, sizeof c->background_feature,
                     "%s", "Background Feature");
        }
    }

    /* Equipment and starting coin. The gear step still offers to take gold
       instead of a package, so this is only what the background itself
       hands over. */
    ui_line("  What equipment does it come with (blank for none)",
            c->background_equipment, sizeof c->background_equipment);
    c->gold += ui_int("  Starting gold from the background", 0, 1000);

    printf("\n  %s: %s\n", c->background_name, c->background_feature);
}

static void choose_background(Character *c)
{
    const char *names[MENU_MAX];
    const char *details[MENU_MAX];
    static char panels[MENU_MAX][1024];
    const char *info[MENU_MAX];
    int i, pick;

    ui_header("Step 4: Describe Your Character -- Background");
    ui_para("Your background describes where you came from and your place in "
            "the world. It grants two skills, sometimes tools or languages, "
            "and a feature.");

    {
        int map[MENU_MAX], n = 0;
        /* One slot is kept back for "Build one of my own" below. */
        for (i = 0; i < BACKGROUND_COUNT && n < MENU_MAX - 1; i++) {
            if (!book_enabled(BACKGROUNDS[i].book)) continue;
            names[n] = BACKGROUNDS[i].name;
            details[n] = BACKGROUNDS[i].feature_summary;
            map[n] = i;
            n++;
        }
        names[n] = "Build one of my own";
        details[n] = "Choose any two skills, two tools or languages, and "
                     "any feature -- the Player's Handbook's own rules for "
                     "customizing a background";
        map[n] = -1;
        n++;

        for (i = 0; i < n; i++) {
            if (map[i] < 0) {
                info[i] = details[i];
            } else {
                background_info(&BACKGROUNDS[map[i]], panels[i],
                                sizeof panels[i]);
                info[i] = panels[i];
            }
        }
        pick = map[ui_menu_info("Backgrounds:", names, details, n, info)];
    }
    if (pick < 0) {
        custom_background(c);
        return;
    }
    c->background_id = pick;

    /* The skills the background simply grants. A slot holding SKL_COUNT is
       one the book leaves to the player, and is filled in below. */
    for (i = 0; i < 2; i++) {
        Skill sk = BACKGROUNDS[pick].skills[i];
        if (sk < SKL_COUNT) {
            c->skill_prof[sk] = 1;
            printf("  Skill gained: %s\n", SKILL_NAME[sk]);
        }
    }

    /* "Persuasion, plus one from among Arcana, History, Nature, and
       Religion" and its like. */
    if (BACKGROUNDS[pick].skill_choice_count > 0) {
        char buf[256];
        const char *opts[MENU_MAX];
        int avail[MENU_MAX], picks[4], n, k;
        n = split_pipe(BACKGROUNDS[pick].skill_choices, buf, sizeof buf,
                       opts, MENU_MAX);
        for (k = 0; k < n; k++) {
            int id = skill_by_name(opts[k]);
            avail[k] = (id >= 0) && !c->skill_prof[id];
        }
        ui_multi("Skill from your background:", opts, avail, n,
                 BACKGROUNDS[pick].skill_choice_count, picks);
        for (k = 0; k < BACKGROUNDS[pick].skill_choice_count; k++) {
            int id;
            if (picks[k] < 0) continue;
            id = skill_by_name(opts[picks[k]]);
            if (id >= 0) {
                c->skill_prof[id] = 1;
                printf("  Skill gained: %s\n", SKILL_NAME[id]);
            }
        }
    }

    grant_tool_line(c, BACKGROUNDS[pick].tool_profs, "Your background");

    for (i = 0; i < BACKGROUNDS[pick].extra_languages; i++) {
        const char *opts[MENU_MAX];
        int avail[MENU_MAX], picks[1], n;
        n = language_choices(c, opts, avail, MENU_MAX);
        ui_multi("Language from your background:", opts, avail, n, 1, picks);
        if (picks[0] >= 0) add_language(c, opts[picks[0]]);
    }
}

/* The one skill a bard, ranger or rogue grants when you multiclass INTO
 * it, and the rogue's thieves' tools.
 *
 * choose_class_skills() below does this at creation and is static and
 * whole-character; a level-up that takes a new class reaches neither, so
 * the free proficiency was granted to a character built as a multiclass
 * and dropped from one that multiclassed later. add_prof_list cannot do it
 * either: it drops any piece containing "skill" on purpose, because
 * choosing one needs a prompt.
 */
void grant_multiclass_extras(Character *c, int class_id)
{
    const ClassData *cd = &CLASSES[class_id];
    const char *opts[SKL_COUNT];
    int avail[SKL_COUNT], picks[2];
    int k, available = 0;
    char prompt[128];

    if (class_id == CLS_ROGUE) add_tool(c, "Thieves' tools");
    if (class_id != CLS_BARD && class_id != CLS_RANGER
        && class_id != CLS_ROGUE) {
        return;
    }
    for (k = 0; k < cd->skill_option_count; k++) {
        opts[k] = SKILL_NAME[cd->skill_options[k]];
        avail[k] = !c->skill_prof[cd->skill_options[k]];
        if (avail[k]) available++;
    }
    if (available < 1) return;

    snprintf(prompt, sizeof prompt, "%s: one skill for multiclassing in:",
             cd->name);
    ui_multi(prompt, opts, avail, cd->skill_option_count, 1, picks);
    if (picks[0] >= 0) c->skill_prof[cd->skill_options[picks[0]]] = 1;
}

/* Class skill proficiencies. Only the class you started in grants these
 * (multiclassing into bard, ranger or rogue grants one, handled here too). */
static void choose_class_skills(Character *c)
{
    int i;

    for (i = 0; i < c->class_count; i++) {
        const ClassData *cd = &CLASSES[c->classes[i].class_id];
        const char *opts[SKL_COUNT];
        int avail[SKL_COUNT], picks[8];
        int n = cd->skill_option_count;
        int picks_wanted = (i == 0) ? cd->skill_picks : 0;
        int k, available = 0;
        char prompt[128];

        /* Multiclassing into these grants one skill from the class list. */
        if (i > 0) {
            int id = c->classes[i].class_id;
            if (id == CLS_BARD || id == CLS_RANGER || id == CLS_ROGUE) {
                picks_wanted = 1;
            }
        }
        if (picks_wanted == 0) continue;

        for (k = 0; k < n; k++) {
            opts[k] = SKILL_NAME[cd->skill_options[k]];
            avail[k] = !c->skill_prof[cd->skill_options[k]];
            if (avail[k]) available++;
        }
        if (available < picks_wanted) picks_wanted = available;
        if (picks_wanted == 0) continue;

        snprintf(prompt, sizeof prompt, "%s skill proficiencies:", cd->name);
        ui_multi(prompt, opts, avail, n, picks_wanted, picks);
        for (k = 0; k < picks_wanted; k++) {
            if (picks[k] >= 0) c->skill_prof[cd->skill_options[picks[k]]] = 1;
        }
    }
}

static void grant_class_tools(Character *c)
{
    int i;
    if (c->class_count > 0) {                   /* multiclass tools vary */
        const ClassData *cd = &CLASSES[c->classes[0].class_id];
        grant_tool_line(c, cd->tool_profs, cd->name);
    }
    /* Rogues gain thieves' tools when multiclassing in as well. */
    for (i = 1; i < c->class_count; i++) {
        if (c->classes[i].class_id == CLS_ROGUE) add_tool(c, "Thieves' tools");
    }
}

/* --------------------------------------------------------------- the wizard */

/* --------------------------------------------------- going back and starting
 *
 * Creation is a fixed sequence of steps, and the two things a player wants
 * part-way through are to undo the last one and to abandon the lot. Both
 * are done by keeping a copy of the character as it stood before each step
 * and jumping out of whatever prompt raised the escape.
 *
 * Undo is a whole-struct copy because a Character is plain data -- no
 * pointers, nothing owned -- so `*c = snapshot` puts back everything a
 * step touched without any step having to know what that was. The
 * alternative, an undo written per step, is a second copy of every step's
 * effects that would rot the first time one of them changed.
 *
 * Nothing on this path allocates or opens a file, so jumping out of the
 * middle of a step leaks nothing. Everything the driver reads after a jump
 * lives in this one static struct rather than in locals, which is what
 * makes the jump safe: a local written between setjmp and longjmp is
 * indeterminate afterwards, and a static is not.
 */
/* The books come first, before anything is chosen, because everything
 * afterwards is drawn from them: a race, a class, a spell or a feat from a
 * book that is switched off is never offered at all. Asked at the end, or
 * left on the main menu for a player to find, it silently decides a build
 * that has already been made.
 */
static void step_books(Character *c)
{
    (void)c;
    ui_header("Step 0: Which Books Are In Play");
    ui_para("Everything the wizard offers comes from the books switched on "
            "here, so this is settled before anything else. The defaults "
            "are the Player's Handbook and the rules most tables use.");
    settings_menu(&SETTINGS);
}

static void step_name(Character *c)
{
    ui_line("\nCharacter name", c->name, sizeof c->name);
    if (!c->name[0]) strcpy(c->name, "Unnamed");
    ui_line("Player name", c->player, sizeof c->player);
}

/* choose_classes is the one step that reports something back to the
   driver, and the level it settles is wanted by build_levels. */
static void step_classes(Character *c);
static void step_abilities(Character *c);
static void wizard_escape(UiEscape e);

static struct {
    jmp_buf step;               /* where a prompt jumps back to */
    Character snap[16];         /* the character before each step */
    /* Which books are in play is not on the character but decides what the
       wizard offers, so going back over the step that sets them has to put
       them back too -- otherwise backing past it leaves a build being made
       from books the player has since turned off. */
    Settings books[16];
    unsigned long rng[16];      /* and the dice before it, so back can undo */
    UiEscape raised;
    int at;                     /* the step being run */
    int target_level;
    int restart;                /* this quit means start again, not leave */
    int keep;                   /* what the confirm screen decided */
} WIZ;

static void step_classes(Character *c)
{
    choose_classes(c, &WIZ.target_level);
}

/* The abilities, the racial bonuses they are added to, and the multiclass
   requirements they have to meet are one step and not three. Racial
   bonuses are applied to scores that must already be chosen, and the
   requirements are checked against the total; entering any of the three
   without the ones before it would check numbers that were not there
   yet. */
static void step_abilities(Character *c)
{
    choose_abilities(c);
    apply_racial_bonuses(c);
    check_multiclass_requirements(c);
}

static void step_profs(Character *c)
{
    grant_class_proficiencies(c);
}

static void step_skills(Character *c)
{
    choose_class_skills(c);
    grant_class_tools(c);
}

static void step_levels(Character *c)
{
    build_levels(c);
}

/* Everything that was decided, before anything is written.
 *
 * The sheet says what the character is; this says what was chosen to make
 * it that, which is the thing worth checking before it goes to a file. The
 * choices list is already kept for the sheet -- every fighting style, pact
 * boon, expertise and skill pick is recorded there as it is made -- so
 * this is a reading of it rather than a second record that could drift.
 */
static void summarise(const Character *c)
{
    int i;

    ui_header("Before This Is Saved");
    printf("  %s", c->name);
    if (c->player[0]) printf("   (played by %s)", c->player);
    printf("\n  %s", ALIGNMENT_NAME[c->alignment]);
    if (c->race_id >= 0) {
        printf(", %s", RACES[c->race_id].name);
        if (c->subrace_id >= 0) printf(" (%s)", SUBRACES[c->subrace_id].name);
    }
    for (i = 0; i < c->class_count; i++) {
        const ClassLevel *cl = &c->classes[i];
        printf("%s%s %d", i ? " / " : ", ", CLASSES[cl->class_id].name,
               cl->level);
        if (cl->subclass_id >= 0) {
            printf(" (%s)", SUBCLASSES[cl->subclass_id].name);
        }
    }
    if (c->background_id >= 0) {
        printf(", %s", BACKGROUNDS[c->background_id].name);
    } else if (c->background_name[0]) {
        printf(", %s", c->background_name);
    }
    printf("\n");

    printf("\n  ");
    for (i = 0; i < ABL_COUNT; i++) {
        int score = ability_score(c, (Ability)i);
        printf("%s %d (%+d)   ", ABILITY_ABBREV[i], score,
               ability_mod_of(score));
    }
    printf("\n  Hit points %d   Armor Class %d   Speed %d feet\n",
           hit_points_max(c), armour_class(c), speed_of(c));

    if (c->language_count) {
        printf("\n  Languages: ");
        for (i = 0; i < c->language_count; i++) {
            printf("%s%s", i ? ", " : "", c->languages[i]);
        }
        printf("\n");
    }
    {
        int wrote = 0;
        for (i = 0; i < SKL_COUNT; i++) {
            if (!c->skill_prof[i]) continue;
            printf("%s%s", wrote ? ", " : "  Skills: ", SKILL_NAME[i]);
            wrote = 1;
        }
        if (wrote) printf("\n");
    }
    if (c->spell_count) printf("  Spells: %d recorded\n", c->spell_count);
    if (c->item_count) printf("  Carrying %d kinds of thing\n", c->item_count);

    if (c->choice_count) {
        printf("\n  What you chose along the way:\n");
        for (i = 0; i < c->choice_count; i++) {
            printf("    %-22s %s\n", c->choices[i].label,
                   c->choices[i].value);
        }
    }
}

static void step_confirm(Character *c)
{
    static const char *const what[] = {
        "Save this character",
        "See the whole sheet first",
        "Go back and change something",
        "Throw it away and start again",
        "Leave without saving"
    };

    for (;;) {
        summarise(c);
        switch (ui_menu("  Is that right?", what, NULL, 5)) {
        case 0:
            WIZ.keep = 1;
            return;
        case 1:
            ui_header("Your Character");
            print_sheet(c);
            break;
        case 2:
            /* The same jump a typed "b" makes, so one piece of machinery
               handles both and the step before this one is re-entered from
               its own snapshot. */
            wizard_escape(UI_ESC_BACK);
            break;
        case 3:
            /* The one quit that does mean start again. A typed q means
               leave, so the two are told apart by this flag rather than by
               the escape, which is the same jump either way. */
            WIZ.restart = 1;
            wizard_escape(UI_ESC_QUIT);
            break;
        default:
            WIZ.keep = 0;
            return;
        }
    }
}

static const struct { const char *name; void (*fn)(Character *); } STEPS[] = {
    { "the books",          step_books },
    { "your name",          step_name },
    { "race",               choose_race },
    { "class",              step_classes },
    { "ability scores",     step_abilities },
    { "proficiencies",      step_profs },
    { "background",         choose_background },
    { "skills",             step_skills },
    { "levels",             step_levels },
    { "equipment",          choose_equipment },
    { "personality",        choose_personality },
    { "the finished sheet", step_confirm },
};
#define STEP_COUNT ((int)(sizeof STEPS / sizeof STEPS[0]))

static void wizard_escape(UiEscape e)
{
    WIZ.raised = e;
    longjmp(WIZ.step, 1);
}

int wizard_create(Character *c)
{
    for (;;) {
        int done = 0;

        /* Both of these belong to the build about to be made rather than to
           the call, so they are cleared per pass: a restarted build starts
           with neither a decision to keep it nor a request to restart. */
        WIZ.keep = 0;
        WIZ.restart = 0;
        memset(c, 0, sizeof *c);
        c->race_id = c->subrace_id = c->background_id = -1;
        c->ancestry_id = -1;
        WIZ.at = 0;
        WIZ.target_level = 1;

        ui_header("Create a D&D 5th Edition Character");
        ui_para("This wizard follows the six steps in chapter 1 of the "
                "Player's Handbook. Everything you choose is saved to a "
                "text file at the end.");
        ui_para("At any menu you can type b to go back one step, q to throw "
                "this character away and return to the main menu, or a "
                "number followed by the word info -- \"3 info\" -- to be "
                "told what that choice means.");

        ui_set_escape(wizard_escape);
        while (WIZ.at < STEP_COUNT) {
            WIZ.snap[WIZ.at] = *c;
            WIZ.books[WIZ.at] = SETTINGS;
            WIZ.rng[WIZ.at] = ui_rng_state();
            if (setjmp(WIZ.step) == 0) {
                STEPS[WIZ.at].fn(c);
                WIZ.at++;
            } else if (WIZ.raised == UI_ESC_BACK) {
                if (WIZ.at == 0) {
                    printf("\n  This is the first step; there is nothing "
                           "behind it.\n");
                } else {
                    WIZ.at--;
                    *c = WIZ.snap[WIZ.at];
                    SETTINGS = WIZ.books[WIZ.at];
                    ui_set_manual_dice(SETTINGS.manual_dice);
                    ui_rng_restore(WIZ.rng[WIZ.at]);
                    printf("\n  Back to %s.\n", STEPS[WIZ.at].name);
                }
            } else {
                break;              /* quit: out to the decision below */
            }
        }
        ui_set_escape(NULL);
        done = (WIZ.at >= STEP_COUNT);
        if (done && WIZ.keep) return 1;

        /* Nothing is being kept, so the books go back the way they were
           before this build started. Step 0 settles which books the
           character draws on, and a character that was thrown away should
           not take the rest of the session's content settings with it.
           All three ways of getting here pass through this: a typed q, the
           confirm screen's "Leave without saving", and its "Throw it away
           and start again", which is where the rollback started life. */
        SETTINGS = WIZ.books[0];
        ui_set_manual_dice(SETTINGS.manual_dice);

        /* A typed q leaves character creation. It used to throw the
           character away and drop the player back at step 0 of a fresh one,
           which is not what "quit" means to anyone who types it: asking to
           stop and being handed a new blank character is the wizard
           refusing to let go. The one quit that does mean start again comes
           from the confirm screen's own entry, which says so in WIZ.restart.
           The caller shows the main menu again when this returns 0. */
        if (done || !WIZ.restart) return 0;

        printf("\n  Throwing that character away and starting again.\n");
    }
}
