/* build.c -- the character creation wizard (PHB chapter 1, steps 1-6). */
#include "build.h"
#include "ui.h"
#include "data_spells.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

enum { CLS_BARBARIAN = 0, CLS_BARD = 1, CLS_CLERIC = 2, CLS_DRUID = 3,
       CLS_FIGHTER = 4, CLS_MONK = 5, CLS_PALADIN = 6, CLS_RANGER = 7,
       CLS_ROGUE = 8, CLS_SORCERER = 9, CLS_WARLOCK = 10, CLS_WIZARD = 11 };

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

/* Case-insensitive substring test. */
static int contains_ci(const char *haystack, const char *needle)
{
    size_t nl = strlen(needle);
    const char *h;

    if (!nl) return 1;
    for (h = haystack; *h; h++) {
        size_t k;
        for (k = 0; k < nl; k++) {
            int a = (unsigned char)h[k], b = (unsigned char)needle[k];
            if (a >= 'A' && a <= 'Z') a += 32;
            if (b >= 'A' && b <= 'Z') b += 32;
            if (!h[k] || a != b) break;
        }
        if (k == nl) return 1;
    }
    return 0;
}

int has_prof(const Character *c, const char *prof)
{
    int i;
    for (i = 0; i < c->other_prof_count; i++) {
        if (contains_ci(c->other_profs[i], prof)) return 1;
    }
    /* A class granting "All armor" is proficient with every armour type. */
    if (contains_ci(prof, "armor")) {
        for (i = 0; i < c->other_prof_count; i++) {
            if (contains_ci(c->other_profs[i], "all armor")) return 1;
        }
    }
    return 0;
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

void add_prof_list(Character *c, const char *csv)
{
    char buf[512], *p, *start;

    if (!csv || !*csv || strcmp(csv, "None") == 0) return;
    strncpy(buf, csv, sizeof buf - 1);
    buf[sizeof buf - 1] = '\0';

    start = buf;
    for (p = buf;; p++) {
        if (*p == ',' || *p == '\0') {
            int end = (*p == '\0');
            *p = '\0';
            while (*start == ' ') start++;
            if (*start) add_prof(c, start);
            if (end) break;
            start = p + 1;
        }
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

void add_item(Character *c, int item_id, int qty, int equipped)
{
    int i;
    if (item_id < 0 || item_id >= ITEM_COUNT || qty <= 0) return;

    for (i = 0; i < c->item_count; i++) {
        if (c->inventory[i].item_id == item_id
            && c->inventory[i].equipped == equipped) {
            c->inventory[i].quantity += qty;
            return;
        }
    }
    if (c->item_count >= MAX_ITEMS) return;
    c->inventory[c->item_count].item_id = item_id;
    c->inventory[c->item_count].quantity = qty;
    c->inventory[c->item_count].equipped = equipped;
    c->item_count++;
}

void add_item_by_name(Character *c, const char *name, int qty, int equipped)
{
    add_item(c, find_item(name), qty, equipped);
}

/* ------------------------------------------------------------------- step 1 */

static void choose_race(Character *c)
{
    const char *names[32];
    const char *details[32];
    int i, pick;

    ui_header("Step 1: Choose a Race");
    ui_para("Your race establishes your general appearance and the natural "
            "talents you gain from ancestry, and increases one or more "
            "ability scores.");

    {
        int map[64], n = 0;
        for (i = 0; i < RACE_COUNT && n < 64; i++) {
            if (!book_enabled(RACES[i].book)) continue;
            names[n] = RACES[i].name;
            details[n] = NULL;
            map[n] = i;
            n++;
        }
        pick = map[ui_menu("Races:", names, details, n)];
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
        int map[32], n = 0;
        for (i = 0; i < total && n < 32; i++) {
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
        char buf[256], *p, *start;
        strncpy(buf, RACES[pick].languages, sizeof buf - 1);
        buf[sizeof buf - 1] = '\0';
        start = buf;
        for (p = buf;; p++) {
            if (*p == ',' || *p == '\0') {
                int end = (*p == '\0');
                *p = '\0';
                while (*start == ' ') start++;
                add_language(c, start);
                if (end) break;
                start = p + 1;
            }
        }
    }

    /* Fixed racial proficiencies that the traits text describes. */
    if (strcmp(RACES[pick].name, "Elf") == 0) c->skill_prof[SKL_PERCEPTION] = 1;
    if (strcmp(RACES[pick].name, "Half-Orc") == 0) {
        c->skill_prof[SKL_INTIMIDATION] = 1;
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
    const char *names[16];
    const char *details[16];
    int class_map[16], class_n;
    int i, remaining;

    ui_header("Step 2: Choose a Class");
    ui_para("Your class describes your vocation, your special talents and the "
            "tactics you are most likely to employ.");

    *target_level = ui_int("\nWhat total character level are you building to",
                           1, MAX_LEVEL);

    {
        int n = 0;
        for (i = 0; i < CLASS_COUNT && n < 16; i++) {
            if (!book_enabled(CLASSES[i].book)) continue;
            names[n] = CLASSES[i].name;
            details[n] = CLASSES[i].quick_build;
            class_map[n] = i;
            n++;
        }
        class_n = n;
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

        pick = class_map[ui_menu(c->class_count == 0
                                     ? "Classes:"
                                     : "Add levels in which class?",
                                 names, details, class_n)];
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
            add_prof_list(c, cd->armour_profs);
            add_prof_list(c, cd->weapon_profs);
            c->save_prof[cd->save_prof[0]] = 1;
            c->save_prof[cd->save_prof[1]] = 1;
        } else {
            add_prof_list(c, cd->mc_profs);
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
        int remaining_min = 0, i, lo = 8, hi = 15;
        char prompt[128];

        /* Reserve nothing for later abilities: every one may stay at 8. */
        for (i = a + 1; i < ABL_COUNT; i++) remaining_min += 0;

        /* Highest score still affordable. */
        while (hi > 8 && POINT_COST[hi] > points - remaining_min) hi--;

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
                pool[i] = roll_4d6_drop_lowest();
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
static void custom_origin_abilities(Character *c, int total)
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

    printf("\n  Customizing Your Origin: your race grants %d points of "
           "ability increase to place as you like.\n", total);

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
            const char *opts[32];
            int avail[32], picks[1], k;
            for (k = 0; k < LANGUAGE_COUNT; k++) {
                opts[k] = LANGUAGES[k];
                avail[k] = !has_language(c, LANGUAGES[k]);
            }
            ui_multi("  Replacement language:", opts, avail, LANGUAGE_COUNT,
                     1, picks);
            if (picks[0] >= 0) add_language(c, LANGUAGES[picks[0]]);
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

    if (SETTINGS.custom_origins) {
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

        custom_origin_abilities(c, total);
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
            const char *opts[32];
            int avail[32], picks[1], i;
            for (i = 0; i < LANGUAGE_COUNT; i++) {
                opts[i] = LANGUAGES[i];
                avail[i] = !has_language(c, LANGUAGES[i]);
            }
            ui_multi("Extra language from your race:", opts, avail,
                     LANGUAGE_COUNT, 1, picks);
            if (picks[0] >= 0) add_language(c, LANGUAGES[picks[0]]);
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

static void choose_background(Character *c)
{
    const char *names[16];
    const char *details[16];
    int i, pick;

    ui_header("Step 4: Describe Your Character -- Background");
    ui_para("Your background describes where you came from and your place in "
            "the world. It grants two skills, sometimes tools or languages, "
            "and a feature.");

    {
        int map[32], n = 0;
        for (i = 0; i < BACKGROUND_COUNT && n < 32; i++) {
            if (!book_enabled(BACKGROUNDS[i].book)) continue;
            names[n] = BACKGROUNDS[i].name;
            details[n] = BACKGROUNDS[i].feature_summary;
            map[n] = i;
            n++;
        }
        pick = map[ui_menu("Backgrounds:", names, details, n)];
    }
    c->background_id = pick;

    c->skill_prof[BACKGROUNDS[pick].skills[0]] = 1;
    c->skill_prof[BACKGROUNDS[pick].skills[1]] = 1;

    printf("\n  Skills gained: %s, %s\n",
           SKILL_NAME[BACKGROUNDS[pick].skills[0]],
           SKILL_NAME[BACKGROUNDS[pick].skills[1]]);

    /* Tool proficiencies: recorded verbatim, since several are "one type of
       ..." and the specific choice is the player's. */
    if (BACKGROUNDS[pick].tool_profs[0]) {
        char buf[256], *p, *start;
        strncpy(buf, BACKGROUNDS[pick].tool_profs, sizeof buf - 1);
        buf[sizeof buf - 1] = '\0';
        start = buf;
        for (p = buf;; p++) {
            if (*p == ',' || *p == '\0') {
                int end = (*p == '\0');
                *p = '\0';
                while (*start == ' ') start++;
                if (*start) {
                    if (strstr(start, "One type of")) {
                        char answer[MAX_NAME];
                        char prompt[MAX_NAME + 320];
                        snprintf(prompt, sizeof prompt,
                                 "  Your background grants \"%s\" -- name it",
                                 start);
                        ui_line(prompt, answer, sizeof answer);
                        add_tool(c, answer[0] ? answer : start);
                    } else {
                        add_tool(c, start);
                    }
                }
                if (end) break;
                start = p + 1;
            }
        }
    }

    for (i = 0; i < BACKGROUNDS[pick].extra_languages; i++) {
        const char *opts[32];
        int avail[32], picks[1], k;
        for (k = 0; k < LANGUAGE_COUNT; k++) {
            opts[k] = LANGUAGES[k];
            avail[k] = !has_language(c, LANGUAGES[k]);
        }
        ui_multi("Language from your background:", opts, avail,
                 LANGUAGE_COUNT, 1, picks);
        if (picks[0] >= 0) add_language(c, LANGUAGES[picks[0]]);
    }
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
    for (i = 0; i < c->class_count; i++) {
        const ClassData *cd = &CLASSES[c->classes[i].class_id];
        if (i > 0) continue;                    /* multiclass tools vary */
        if (!cd->tool_profs[0]) continue;

        if (strstr(cd->tool_profs, "of your choice")) {
            char answer[MAX_NAME], prompt[200];
            snprintf(prompt, sizeof prompt, "  %s grants \"%s\" -- name it",
                     cd->name, cd->tool_profs);
            ui_line(prompt, answer, sizeof answer);
            add_tool(c, answer[0] ? answer : cd->tool_profs);
        } else {
            add_tool(c, cd->tool_profs);
        }
    }
    /* Rogues gain thieves' tools when multiclassing in as well. */
    for (i = 1; i < c->class_count; i++) {
        if (c->classes[i].class_id == CLS_ROGUE) add_tool(c, "Thieves' tools");
    }
}

/* --------------------------------------------------------------- the wizard */

void wizard_create(Character *c)
{
    int target_level;

    memset(c, 0, sizeof *c);
    c->race_id = c->subrace_id = c->background_id = -1;
    c->ancestry_id = -1;

    ui_header("Create a D&D 5th Edition Character");
    ui_para("This wizard follows the six steps in chapter 1 of the Player's "
            "Handbook. Everything you choose is saved to a text file at the "
            "end.");

    ui_line("\nCharacter name", c->name, sizeof c->name);
    if (!c->name[0]) strcpy(c->name, "Unnamed");
    ui_line("Player name", c->player, sizeof c->player);

    choose_race(c);
    choose_classes(c, &target_level);
    choose_abilities(c);
    apply_racial_bonuses(c);
    check_multiclass_requirements(c);

    grant_class_proficiencies(c);
    choose_background(c);
    choose_class_skills(c);
    grant_class_tools(c);

    build_levels(c);
    choose_equipment(c);
    choose_personality(c);
}
