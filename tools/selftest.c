/* selftest.c -- assertions on the derived-statistics engine.
 *
 * Builds characters in memory and checks the numbers the PHB says they
 * should have. Build and run with "make test".
 */
#include "dnd.h"
#include "data.h"
#include "sidekick.h"
#include "build.h"
#include "data_spells.h"
#include "saveload.h"
#include "homebrew.h"

#include <stdio.h>
#include <string.h>



static int failures;

static void check(int ok, const char *what, long got, long want)
{
    if (ok) return;
    printf("  FAIL %-52s got %ld, want %ld\n", what, got, want);
    failures++;
}

#define EQ(expr, want, what) do {                                           \
        long g_ = (long)(expr), w_ = (long)(want);                          \
        check(g_ == w_, what, g_, w_);                                      \
    } while (0)

static void reset(Character *c)
{
    memset(c, 0, sizeof *c);
    c->race_id = c->subrace_id = c->background_id = c->ancestry_id = -1;
    c->classes[0].subclass_id = -1;
}

static void add_class(Character *c, int id, int level, int subclass)
{
    int k = c->class_count++;
    c->classes[k].class_id = id;
    c->classes[k].level = level;
    c->classes[k].subclass_id = subclass;
    c->classes[k].subclass_option = -1;
}

static int find_race(const char *n)
{
    int i;
    for (i = 0; i < RACE_COUNT; i++) if (!strcmp(RACES[i].name, n)) return i;
    return -1;
}
static int find_subrace(const char *n)
{
    int i;
    for (i = 0; i < SUBRACE_COUNT; i++) if (!strcmp(SUBRACES[i].name, n)) return i;
    return -1;
}

/* ------------------------------------------------------------------ tests */

static void test_modifiers(void)
{
    printf("ability modifiers\n");
    EQ(ability_mod_of(1), -5, "score 1");
    EQ(ability_mod_of(3), -4, "score 3");
    EQ(ability_mod_of(8), -1, "score 8");
    EQ(ability_mod_of(9), -1, "score 9");
    EQ(ability_mod_of(10), 0, "score 10");
    EQ(ability_mod_of(11), 0, "score 11");
    EQ(ability_mod_of(15), 2, "score 15");
    EQ(ability_mod_of(20), 5, "score 20");
    EQ(ability_mod_of(30), 10, "score 30");
}

static void test_proficiency(void)
{
    Character c;
    int lvl;
    static const int want[21] = {0, 2,2,2,2, 3,3,3,3, 4,4,4,4,
                                 5,5,5,5, 6,6,6,6};
    printf("proficiency bonus\n");
    for (lvl = 1; lvl <= 20; lvl++) {
        char buf[48];
        reset(&c);
        add_class(&c, CLS_FIGHTER, lvl, -1);
        snprintf(buf, sizeof buf, "level %d", lvl);
        EQ(proficiency_bonus(&c), want[lvl], buf);
    }
}

/* The PHB's own worked example: Bruenor, a 1st-level mountain dwarf fighter
 * with the standard array, ends on 17/10/16/8/13/12 and 13 hit points. */
static void test_bruenor(void)
{
    Character c;

    printf("Bruenor (PHB chapter 1 example)\n");
    reset(&c);
    c.race_id = find_race("Dwarf");
    c.subrace_id = find_subrace("Mountain Dwarf");
    add_class(&c, CLS_FIGHTER, 1, -1);

    c.base_score[ABL_STR] = 15; c.base_score[ABL_DEX] = 10;
    c.base_score[ABL_CON] = 14; c.base_score[ABL_INT] = 8;
    c.base_score[ABL_WIS] = 13; c.base_score[ABL_CHA] = 12;
    c.racial_bonus[ABL_CON] = 2;    /* dwarf */
    c.racial_bonus[ABL_STR] = 2;    /* mountain dwarf */
    c.hp_rolls[0] = 10; c.hp_roll_count = 1;

    EQ(ability_score(&c, ABL_STR), 17, "Strength");
    EQ(ability_score(&c, ABL_CON), 16, "Constitution");
    EQ(ability_mod(&c, ABL_STR), 3, "Strength modifier");
    EQ(ability_mod(&c, ABL_CON), 3, "Constitution modifier");
    EQ(hit_points_max(&c), 13, "hit points (10 + Con 3)");
    EQ(speed_of(&c), 25, "dwarf speed");
    EQ(carrying_capacity(&c), 255, "carrying capacity (Str 17 x 15)");
}

static void test_hill_dwarf_and_tough(void)
{
    Character c;
    int tough = -1, i;

    printf("hit point bonuses\n");
    reset(&c);
    c.race_id = find_race("Dwarf");
    c.subrace_id = find_subrace("Hill Dwarf");
    add_class(&c, CLS_FIGHTER, 5, -1);
    for (i = 0; i < 5; i++) c.hp_rolls[i] = 6;
    c.hp_roll_count = 5;
    c.base_score[ABL_CON] = 14;     /* +2 */

    /* 30 rolled + 5 x Con 2 + 5 from Dwarven Toughness. */
    EQ(hit_points_max(&c), 45, "hill dwarf 5th level");

    for (i = 0; i < FEAT_COUNT; i++) if (!strcmp(FEATS[i].name, "Tough")) tough = i;
    c.feats[c.feat_count++] = tough;
    EQ(hit_points_max(&c), 55, "with the Tough feat (+2 per level)");
}

static void test_armour_class(void)
{
    Character c;

    printf("armor class\n");

    /* Unarmoured, Dex 14. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    c.base_score[ABL_DEX] = 14;
    EQ(armour_class(&c), 12, "unarmoured, Dex 14");

    /* Chain mail: fixed 16, Dexterity ignored. */
    add_item_by_name(&c, "Chain mail", 1, 1);
    EQ(armour_class(&c), 16, "chain mail");

    /* Plus a shield. */
    add_item_by_name(&c, "Shield", 1, 1);
    EQ(armour_class(&c), 18, "chain mail and shield");

    /* Leather armour caps nothing: 11 + full Dex. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    c.base_score[ABL_DEX] = 18;
    add_item_by_name(&c, "Leather armor", 1, 1);
    EQ(armour_class(&c), 15, "leather armor, Dex 18");

    /* Half plate caps the Dexterity bonus at +2. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    c.base_score[ABL_DEX] = 18;
    add_item_by_name(&c, "Half plate", 1, 1);
    EQ(armour_class(&c), 17, "half plate caps Dex at +2");

    /* Barbarian Unarmored Defense: 10 + Dex + Con. */
    reset(&c);
    add_class(&c, CLS_BARBARIAN, 1, -1);
    c.base_score[ABL_DEX] = 14; c.base_score[ABL_CON] = 16;
    EQ(armour_class(&c), 15, "barbarian unarmored defense");

    /* Monk Unarmored Defense: 10 + Dex + Wis. */
    reset(&c);
    add_class(&c, CLS_MONK, 1, -1);
    c.base_score[ABL_DEX] = 16; c.base_score[ABL_WIS] = 14;
    EQ(armour_class(&c), 15, "monk unarmored defense");

    /* Draconic Resilience: 13 + Dex. */
    reset(&c);
    add_class(&c, CLS_SORCERER, 1, subclass_by_name("Draconic Bloodline"));
    c.base_score[ABL_DEX] = 12;
    EQ(armour_class(&c), 14, "draconic sorcerer unarmoured");
}

static void test_spell_slots(void)
{
    Character c;
    int slots[10];

    printf("spell slots\n");

    /* A 5th-level wizard: 4/3/2. */
    reset(&c);
    add_class(&c, CLS_WIZARD, 5, -1);
    spell_slots_for(&c, slots);
    EQ(slots[1], 4, "wizard 5, 1st level");
    EQ(slots[2], 3, "wizard 5, 2nd level");
    EQ(slots[3], 2, "wizard 5, 3rd level");

    /* A 5th-level paladin uses the half-caster table: 4/2. */
    reset(&c);
    add_class(&c, CLS_PALADIN, 5, -1);
    spell_slots_for(&c, slots);
    EQ(slots[1], 4, "paladin 5, 1st level");
    EQ(slots[2], 2, "paladin 5, 2nd level");

    /* Paladin 5 with a non-casting class still uses the paladin table:
     * the PHB's multiclass table applies only when Spellcasting comes from
     * more than one class. */
    reset(&c);
    add_class(&c, CLS_PALADIN, 5, -1);
    add_class(&c, CLS_ROGUE, 1, -1);
    spell_slots_for(&c, slots);
    EQ(slots[1], 4, "paladin 5 / rogue 1, 1st level");
    EQ(slots[2], 2, "paladin 5 / rogue 1, 2nd level");

    /* Two casting classes: half levels round down, so paladin 5 counts 2,
     * plus sorcerer 1, giving caster level 3 -> 4/2. */
    reset(&c);
    add_class(&c, CLS_PALADIN, 5, -1);
    add_class(&c, CLS_SORCERER, 1, -1);
    EQ(caster_level(&c), 3, "paladin 5 / sorcerer 1 caster level");
    spell_slots_for(&c, slots);
    EQ(slots[1], 4, "paladin 5 / sorcerer 1, 1st level");
    EQ(slots[2], 2, "paladin 5 / sorcerer 1, 2nd level");

    /* Wizard 3 / cleric 3 -> caster level 6 -> 4/3/3. */
    reset(&c);
    add_class(&c, CLS_WIZARD, 3, -1);
    add_class(&c, CLS_CLERIC, 3, -1);
    EQ(caster_level(&c), 6, "wizard 3 / cleric 3 caster level");
    spell_slots_for(&c, slots);
    EQ(slots[1], 4, "wizard 3 / cleric 3, 1st level");
    EQ(slots[2], 3, "wizard 3 / cleric 3, 2nd level");
    EQ(slots[3], 3, "wizard 3 / cleric 3, 3rd level");

    /* Eldritch Knight is a third-caster: fighter 3 -> caster level 1. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 3, subclass_by_name("Eldritch Knight"));
    EQ(caster_level(&c), 1, "eldritch knight 3 caster level");
    spell_slots_for(&c, slots);
    EQ(slots[1], 2, "eldritch knight 3, 1st level");

    /* Paladins and rangers have no spell slots at 1st level. */
    reset(&c);
    add_class(&c, CLS_PALADIN, 1, -1);
    EQ(spell_slots_for(&c, slots), 0, "paladin 1 has no slots");
    reset(&c);
    add_class(&c, CLS_RANGER, 1, -1);
    EQ(spell_slots_for(&c, slots), 0, "ranger 1 has no slots");

    /* The artificer, by contrast, casts from 1st level. */
    reset(&c);
    add_class(&c, CLS_ARTIFICER, 1, -1);
    spell_slots_for(&c, slots);
    EQ(slots[1], 2, "artificer 1, 1st level");
    reset(&c);
    add_class(&c, CLS_ARTIFICER, 5, -1);
    spell_slots_for(&c, slots);
    EQ(slots[1], 4, "artificer 5, 1st level");
    EQ(slots[2], 2, "artificer 5, 2nd level");
    reset(&c);
    add_class(&c, CLS_ARTIFICER, 20, -1);
    spell_slots_for(&c, slots);
    EQ(slots[5], 2, "artificer 20, 5th level");

    /* Tasha's has the artificer round UP when multiclassing, where the
     * paladin and ranger round down. */
    reset(&c);
    add_class(&c, CLS_ARTIFICER, 5, -1);
    add_class(&c, CLS_WIZARD, 1, -1);
    EQ(caster_level(&c), 4, "artificer 5 / wizard 1 rounds up");
    reset(&c);
    add_class(&c, CLS_PALADIN, 5, -1);
    add_class(&c, CLS_WIZARD, 1, -1);
    EQ(caster_level(&c), 3, "paladin 5 / wizard 1 rounds down");

    /* A plain fighter has none. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 10, -1);
    EQ(spell_slots_for(&c, slots), 0, "fighter has no slots");
}

static void test_artificer(void)
{
    Character c;

    printf("artificer\n");
    reset(&c);
    add_class(&c, CLS_ARTIFICER, 5, -1);
    c.base_score[ABL_INT] = 16;                 /* +3 */
    /* Intelligence modifier + half artificer level, rounded down. */
    EQ(spells_prepared_count(&c, CLS_ARTIFICER), 5, "prepared at 5th level");
    EQ(known_spell_count(&c, CLS_ARTIFICER, 1), 2, "cantrips at 5th level");

    reset(&c);
    add_class(&c, CLS_ARTIFICER, 14, -1);
    c.base_score[ABL_INT] = 20;                 /* +5 */
    EQ(spells_prepared_count(&c, CLS_ARTIFICER), 12, "prepared at 14th level");
    EQ(known_spell_count(&c, CLS_ARTIFICER, 1), 4, "cantrips at 14th level");
    EQ(INFUSIONS_KNOWN[14], 10, "infusions known at 14th level");
    EQ(INFUSED_ITEMS[14], 5, "infused items at 14th level");
    EQ(INFUSIONS_KNOWN[2], 4, "infusions known at 2nd level");
    EQ(INFUSED_ITEMS[2], 2, "infused items at 2nd level");
}

static void test_pact_magic(void)
{
    Character c;
    int n, lvl;

    printf("pact magic\n");
    reset(&c);
    add_class(&c, CLS_WARLOCK, 1, -1);
    pact_slots_for(&c, &n, &lvl);
    EQ(n, 1, "warlock 1 slot count");
    EQ(lvl, 1, "warlock 1 slot level");

    reset(&c);
    add_class(&c, CLS_WARLOCK, 11, -1);
    pact_slots_for(&c, &n, &lvl);
    EQ(n, 3, "warlock 11 slot count");
    EQ(lvl, 5, "warlock 11 slot level");

    reset(&c);
    add_class(&c, CLS_WARLOCK, 20, -1);
    pact_slots_for(&c, &n, &lvl);
    EQ(n, 4, "warlock 20 slot count");
    EQ(lvl, 5, "warlock 20 slot level");
}

static void test_skills_and_saves(void)
{
    Character c;

    printf("skills and saving throws\n");
    reset(&c);
    add_class(&c, CLS_ROGUE, 5, -1);
    c.base_score[ABL_DEX] = 16;         /* +3 */
    c.skill_prof[SKL_STEALTH] = 1;
    EQ(proficiency_bonus(&c), 3, "level 5 proficiency");
    EQ(skill_bonus(&c, SKL_STEALTH), 6, "Stealth, proficient");
    c.skill_expertise[SKL_STEALTH] = 1;
    EQ(skill_bonus(&c, SKL_STEALTH), 9, "Stealth, expertise");
    EQ(skill_bonus(&c, SKL_ACROBATICS), 3, "Acrobatics, not proficient");

    /* A bard of 2nd level adds half proficiency to everything else. */
    reset(&c);
    add_class(&c, CLS_BARD, 2, -1);
    c.base_score[ABL_DEX] = 14;         /* +2 */
    EQ(skill_bonus(&c, SKL_ACROBATICS), 3, "Jack of All Trades");

    /* A monk of 14th level is proficient in every save. */
    reset(&c);
    add_class(&c, CLS_MONK, 14, -1);
    c.base_score[ABL_INT] = 10;
    EQ(save_bonus(&c, ABL_INT), 5, "monk 14 Diamond Soul");
}

static void test_spell_data(void)
{
    int i, cantrips = 0, rituals = 0, conc = 0, noclass = 0;

    printf("spell data\n");
    /* 361 from the Player's Handbook, 95 from Xanathar's Guide and 21 from
     * Tasha's Cauldron. */
    EQ(SPELL_COUNT, 477, "spell count across the three books");
    for (i = 0; i < SPELL_COUNT; i++) {
        if (SPELLS[i].level == 0) cantrips++;
        if (SPELLS[i].ritual) rituals++;
        if (SPELLS[i].concentration) conc++;
        if (SPELLS[i].classes == 0) noclass++;
        if (!SPELLS[i].name[0] || !SPELLS[i].casting_time[0]
            || !SPELLS[i].range[0] || !SPELLS[i].duration[0]) {
            printf("  FAIL spell %d has an empty field\n", i);
            failures++;
        }
    }
    EQ(cantrips, 44, "cantrip count");
    EQ(noclass, 0, "spells on no class list");
    EQ(rituals, 33, "ritual spell count");
    EQ(conc, 218, "concentration spell count");
}

static void test_data_integrity(void)
{
    int i;

    printf("data table integrity\n");
    {
        int phb = 0, mtf = 0, k;
        for (k = 0; k < RACE_COUNT; k++) {
            if (RACES[k].book == BOOK_PHB) phb++;
            if (RACES[k].book == BOOK_MPMM) mtf++;
        }
        EQ(phb, 9, "PHB races");
        EQ(mtf, 33, "Monsters of the Multiverse races");
        EQ(RACE_COUNT, phb + mtf, "every race is from the PHB or Multiverse");
    }
    {
        int phb = 0, scag = 0, k;
        for (k = 0; k < SUBRACE_COUNT; k++) {
            if (SUBRACES[k].book == BOOK_PHB) phb++;
            if (SUBRACES[k].book == BOOK_SCAG) scag++;
        }
        EQ(phb, 11, "PHB subraces");
        EQ(scag, 19, "SCAG subraces");
        EQ(SUBRACE_COUNT, phb + scag, "every subrace is from the PHB or SCAG");

        /* A race reaches its subraces through a window into the table, so
           every subrace has to fall inside exactly one race's window: one
           written out of place would be unreachable, or would show up
           under the wrong race. */
        for (k = 0; k < SUBRACE_COUNT; k++) {
            int r, owners = 0;
            for (r = 0; r < RACE_COUNT; r++) {
                if (k >= RACES[r].first_subrace
                    && k < RACES[r].first_subrace + RACES[r].subrace_count) {
                    owners++;
                }
            }
            if (owners != 1) {
                printf("  FAIL subrace \"%s\" is claimed by %d races\n",
                       SUBRACES[k].name, owners);
                failures++;
            }
        }
    }
    {
        int k;

        /* The Multiverse races have no fixed increases at all; the spread
           is always chosen, so each must say so. */
        for (k = 0; k < RACE_COUNT; k++) {
            int a, total = 0;
            for (a = 0; a < ABL_COUNT; a++) total += RACES[k].ability[a];
            if (RACES[k].book == BOOK_MPMM) {
                if (total != 0 || !RACES[k].origin_choice) {
                    printf("  FAIL %s should have no fixed increases\n",
                           RACES[k].name);
                    failures++;
                }
            } else if (RACES[k].origin_choice) {
                printf("  FAIL %s should not choose its own spread\n",
                       RACES[k].name);
                failures++;
            }
        }
    }
    EQ(CLASS_COUNT, 13, "classes (12 PHB + the artificer)");
    EQ(BACKGROUND_COUNT, 26,
       "backgrounds, 13 from the PHB and 13 from SCAG");
    {
        int phb = 0, scag = 0, k;
        for (k = 0; k < BACKGROUND_COUNT; k++) {
            if (BACKGROUNDS[k].book == BOOK_PHB) phb++;
            if (BACKGROUNDS[k].book == BOOK_SCAG) scag++;
        }
        EQ(phb, 13, "PHB backgrounds");
        EQ(scag, 13,
           "SCAG backgrounds, the City Watch investigator among them");
    }
    {
        /* Every background hands out exactly two skills, whether it names
           them or leaves them to the player, and every name it offers has
           to be a skill that exists. */
        int k, with_choices = 0;
        for (k = 0; k < BACKGROUND_COUNT; k++) {
            const BackgroundData *b = &BACKGROUNDS[k];
            int fixed = 0, q, np;
            char buf[256];
            const char *parts[24];

            for (q = 0; q < 2; q++) if (b->skills[q] < SKL_COUNT) fixed++;
            if (fixed + b->skill_choice_count != 2) {
                printf("  FAIL background \"%s\" grants %d skills, not 2\n",
                       b->name, fixed + b->skill_choice_count);
                failures++;
            }
            if (!b->skill_choice_count) {
                if (b->skill_choices[0]) {
                    printf("  FAIL background \"%s\" offers skills it never "
                           "asks for\n", b->name);
                    failures++;
                }
                continue;
            }
            with_choices++;
            np = split_pipe(b->skill_choices, buf, sizeof buf, parts, 24);
            if (np <= b->skill_choice_count) {
                printf("  FAIL background \"%s\" offers %d skills to choose "
                       "%d from\n", b->name, np, b->skill_choice_count);
                failures++;
            }
            for (q = 0; q < np; q++) {
                if (skill_by_name(parts[q]) < 0) {
                    printf("  FAIL background \"%s\" offers \"%s\", which is "
                           "not a skill\n", b->name, parts[q]);
                    failures++;
                }
            }
        }
        EQ(with_choices, 5, "SCAG backgrounds that leave a skill to you");
    }
    {
        int phb = 0, tce = 0, xge = 0, scag = 0, k;
        for (k = 0; k < FEAT_COUNT; k++) {
            if (FEATS[k].book == BOOK_PHB)  phb++;
            if (FEATS[k].book == BOOK_TCE)  tce++;
            if (FEATS[k].book == BOOK_XGE)  xge++;
            if (FEATS[k].book == BOOK_SCAG) scag++;
        }
        EQ(phb, 42, "PHB feats");
        EQ(tce, 15, "Tasha's feats");
        EQ(xge, 15, "Xanathar's racial feats");
        EQ(scag, 1, "SCAG's deep gnome feat");
        EQ(FEAT_COUNT, phb + tce + xge + scag,
           "every feat comes from a book");

        /* Every racial feat must name races that exist, or it could never
           be offered to anyone. */
        for (k = 0; k < FEAT_COUNT; k++) {
            char buf[128];
            const char *parts[8];
            int np, q;
            if (!FEATS[k].req_race[0]) continue;
            np = split_pipe(FEATS[k].req_race, buf, sizeof buf, parts, 8);
            for (q = 0; q < np; q++) {
                int found = find_race(parts[q]) >= 0
                         || find_subrace(parts[q]) >= 0;
                if (!found) {
                    printf("  FAIL feat \"%s\" needs race \"%s\", which "
                           "does not exist\n", FEATS[k].name, parts[q]);
                    failures++;
                }
            }
        }
    }

    /* Every class must offer at least one subclass, and every subclass must
     * point back at a real class. */
    for (i = 0; i < CLASS_COUNT; i++) {
        int ids[64];
        int n = subclasses_of(i, ids, 64);
        char buf[64];
        snprintf(buf, sizeof buf, "%s has subclasses", CLASSES[i].name);
        check(n > 0, buf, n, 1);
    }
    for (i = 0; i < SUBCLASS_COUNT; i++) {
        int k, features = 0;
        if (SUBCLASSES[i].class_id < 0 || SUBCLASSES[i].class_id >= CLASS_COUNT) {
            printf("  FAIL subclass %d (%s) names no real class\n", i,
                   SUBCLASSES[i].name);
            failures++;
        }
        /* A subclass with no features would be selectable but empty, which
         * is how a mistyped index in data_features.c shows up. */
        for (k = 0; k < FEATURE_COUNT; k++) {
            if (FEATURES[k].subclass_id == i) features++;
        }
        if (features == 0) {
            printf("  FAIL subclass %d (%s) has no features\n", i,
                   SUBCLASSES[i].name);
            failures++;
        }
    }
    EQ(SUBCLASS_COUNT, 107, "subclasses across the four books");
    {
        int by_book[BOOK_COUNT], k;
        for (k = 0; k < BOOK_COUNT; k++) by_book[k] = 0;
        for (k = 0; k < SUBCLASS_COUNT; k++) by_book[SUBCLASSES[k].book]++;
        EQ(by_book[BOOK_PHB], 40, "PHB subclasses");
        EQ(by_book[BOOK_XGE], 31, "XGE subclasses");
        EQ(by_book[BOOK_TCE], 30, "TCE subclasses");
        EQ(by_book[BOOK_SCAG], 6,
           "SCAG subclasses, the five XGE reprinted not counted twice");
    }
    /* Every feature must name a class that exists and a level in range. */
    for (i = 0; i < FEATURE_COUNT; i++) {
        if (FEATURES[i].class_id < 0 || FEATURES[i].class_id >= CLASS_COUNT
            || FEATURES[i].level < 1 || FEATURES[i].level > MAX_LEVEL) {
            printf("  FAIL feature %d (%s) is out of range\n", i,
                   FEATURES[i].name);
            failures++;
        }
        if (FEATURES[i].subclass_id != -1
            && SUBCLASSES[FEATURES[i].subclass_id].class_id
               != FEATURES[i].class_id) {
            printf("  FAIL feature %d (%s) names another class's subclass\n",
                   i, FEATURES[i].name);
            failures++;
        }
    }
    /* Armour must be findable by name, since the wizard looks it up that way. */
    check(find_item("Chain mail") >= 0, "find Chain mail",
          find_item("Chain mail"), 0);
    check(find_item("chain mail") >= 0, "find chain mail (case-insensitive)",
          find_item("chain mail"), 0);
}

/* Every spell named in Tasha's "Additional <Class> Spells" lists, and in the
 * subclass spell grants, must exist in the database -- a typo there would
 * silently drop a spell the character is entitled to. */
static int spell_named(const char *name, size_t len)
{
    int i;
    for (i = 0; i < SPELL_COUNT; i++) {
        size_t j;
        const char *a = SPELLS[i].name;
        if (strlen(a) != len) continue;
        for (j = 0; j < len; j++) {
            int ca = a[j], cb = name[j];
            if (ca >= 'A' && ca <= 'Z') ca += 32;
            if (cb >= 'A' && cb <= 'Z') cb += 32;
            if (ca != cb) break;
        }
        if (j == len) return 1;
    }
    return 0;
}

static void check_spell_list(const char *list, const char *what)
{
    const char *p = list;

    while (p && *p) {
        const char *end;
        size_t len;
        while (*p == ' ' || *p == ',') p++;
        if (!*p) break;
        end = strchr(p, ',');
        len = end ? (size_t)(end - p) : strlen(p);
        while (len && p[len - 1] == ' ') len--;
        if (len && !spell_named(p, len)) {
            printf("  FAIL %s names no spell: \"%.*s\"\n", what,
                   (int)len, p);
            failures++;
        }
        p = end ? end + 1 : NULL;
    }
}

static void test_expansion_data(void)
{
    int i;

    printf("expansion data\n");
    EQ(OPTIONAL_FEATURE_COUNT, 42, "Tasha's optional class features");
    EQ(ADDITIONAL_SPELLS_COUNT, 8, "classes with an additional spell list");
    EQ(INFUSION_COUNT, 16, "artificer infusions");

    for (i = 0; i < OPTIONAL_FEATURE_COUNT; i++) {
        if (OPTIONAL_FEATURES[i].class_id < 0
            || OPTIONAL_FEATURES[i].class_id >= CLASS_COUNT
            || OPTIONAL_FEATURES[i].level < 1
            || OPTIONAL_FEATURES[i].level > MAX_LEVEL) {
            printf("  FAIL optional feature %d (%s) is out of range\n", i,
                   OPTIONAL_FEATURES[i].name);
            failures++;
        }
    }
    for (i = 0; i < ADDITIONAL_SPELLS_COUNT; i++) {
        char what[64];
        snprintf(what, sizeof what, "%s additional spells",
                 CLASSES[ADDITIONAL_SPELLS[i].class_id].name);
        check_spell_list(ADDITIONAL_SPELLS[i].spells, what);
    }
    /* Subclass spell grants use the same '|' separated, comma separated form. */
    for (i = 0; i < SUBCLASS_COUNT; i++) {
        char buf[1024], what[80];
        const char *p;
        if (!SUBCLASSES[i].bonus_spells[0]) continue;
        snprintf(what, sizeof what, "%s spells", SUBCLASSES[i].name);
        strncpy(buf, SUBCLASSES[i].bonus_spells, sizeof buf - 1);
        buf[sizeof buf - 1] = '\0';
        for (p = buf; *p; ) {
            char *bar = strchr((char *)p, '|');
            if (bar) *bar = '\0';
            check_spell_list(p, what);
            if (!bar) break;
            p = bar + 1;
        }
    }
    for (i = 0; i < INFUSION_COUNT; i++) {
        if (INFUSIONS[i].min_level < 1 || INFUSIONS[i].min_level > MAX_LEVEL) {
            printf("  FAIL infusion %s has a bad minimum level\n",
                   INFUSIONS[i].name);
            failures++;
        }
    }
}

/* -------------------------------------------------- items and option lists */

static void test_item_reference(void)
{
    int i, notes_matched = 0;

    printf("item reference\n");

    /* Every note must name a real catalogue item, or the lookup silently
       never fires. */
    for (i = 0; i < ITEM_NOTE_COUNT; i++) {
        if (find_item(ITEM_NOTES[i].item) >= 0) {
            notes_matched++;
        } else {
            printf("  FAIL item note names no catalogue item: \"%s\"\n",
                   ITEM_NOTES[i].item);
            failures++;
        }
        if (!ITEM_NOTES[i].text || !ITEM_NOTES[i].text[0]) {
            printf("  FAIL item note is empty: \"%s\"\n", ITEM_NOTES[i].item);
            failures++;
        }
    }
    EQ(notes_matched, ITEM_NOTE_COUNT, "item notes resolve to catalogue items");

    /* Armour, weapons and tools should all have something to say. */
    check(item_notes("Plate armor") != NULL, "plate armor has a note", 1, 1);
    check(item_notes("Thieves' tools") != NULL, "thieves' tools have a note",
          1, 1);
    check(item_notes("Healer's kit") != NULL, "healer's kit has a note", 1, 1);

    EQ(WEAPON_PROPERTY_COUNT, 11, "weapon properties explained");
    EQ(TRINKET_COUNT, 100, "trinkets");
    EQ(LIFESTYLE_COUNT, 7, "lifestyles");
    check(SERVICE_COUNT > 20, "food, lodging and services priced",
          SERVICE_COUNT, 21);

    /* Magic items: every one needs a type, a rarity and a description, and
       the artifacts must all be present. */
    check(MAGIC_ITEM_COUNT > 250, "magic items", MAGIC_ITEM_COUNT, 251);
    for (i = 0; i < MAGIC_ITEM_COUNT; i++) {
        const MagicItem *m = &MAGIC_ITEMS[i];
        if (!m->type[0] || !m->rarity[0] || !m->text[0]) {
            printf("  FAIL magic item is incomplete: \"%s\"\n", m->name);
            failures++;
        }
        if (i > 0 && strcmp(MAGIC_ITEMS[i - 1].name, m->name) == 0) {
            printf("  FAIL magic item listed twice: \"%s\"\n", m->name);
            failures++;
        }
    }
    check(find_magic_item("Deck of Many Things") >= 0, "find a magic item by "
          "name", 1, 1);
    check(find_magic_item("Bag of Holding") >= 0, "find bag of holding", 1, 1);
    check(find_magic_item("No Such Item") < 0, "unknown magic item is not "
          "found", 1, 1);
}

static void test_option_lists(void)
{
    int i, j;

    printf("class option lists\n");
    check(OPTION_LIST_COUNT >= 9, "class option lists", OPTION_LIST_COUNT, 9);

    for (i = 0; i < OPTION_LIST_COUNT; i++) {
        const OptionList *ol = &OPTION_LISTS[i];
        int cls = -1, prev = 0;

        for (j = 0; j < CLASS_COUNT; j++) {
            if (strcmp(CLASSES[j].name, ol->class_name) == 0) cls = j;
        }
        if (cls < 0) {
            printf("  FAIL option list names no class: \"%s\"\n",
                   ol->class_name);
            failures++;
        }
        if (ol->subclass_name[0]) {
            int found = 0;
            for (j = 0; j < SUBCLASS_COUNT; j++) {
                if (strcmp(SUBCLASSES[j].name, ol->subclass_name) == 0) found = 1;
            }
            if (!found) {
                printf("  FAIL option list names no subclass: \"%s\"\n",
                       ol->subclass_name);
                failures++;
            }
        }

        /* A class never forgets an option, and never knows more than exist. */
        for (j = 0; j <= MAX_LEVEL; j++) {
            if (ol->known[j] < prev) {
                printf("  FAIL %s known count falls at level %d\n",
                       ol->label, j);
                failures++;
            }
            if (ol->known[j] > ol->count) {
                printf("  FAIL %s knows %d of only %d at level %d\n",
                       ol->label, (int)ol->known[j], ol->count, j);
                failures++;
            }
            prev = ol->known[j];
        }

        for (j = 0; j < ol->count; j++) {
            if (!ol->options[j].name[0]) {
                printf("  FAIL %s has an unnamed entry\n", ol->label);
                failures++;
            }
            if (ol->options[j].min_level > MAX_LEVEL) {
                printf("  FAIL %s entry \"%s\" is never reachable\n",
                       ol->label, ol->options[j].name);
                failures++;
            }
        }
    }

    /* The counts a player would notice. */
    for (i = 0; i < OPTION_LIST_COUNT; i++) {
        const OptionList *ol = &OPTION_LISTS[i];
        if (strcmp(ol->label, "Eldritch Invocation") == 0) {
            EQ(ol->known[2], 2, "warlock knows 2 invocations at 2nd level");
            EQ(ol->known[20], 8, "warlock knows 8 invocations at 20th level");
        }
        if (strcmp(ol->label, "Metamagic option") == 0) {
            EQ(ol->known[3], 2, "sorcerer knows 2 metamagic at 3rd level");
            EQ(ol->known[17], 4, "sorcerer knows 4 metamagic at 17th level");
        }
        if (strcmp(ol->label, "Maneuver") == 0) {
            EQ(ol->known[3], 3, "battle master knows 3 maneuvers at 3rd");
            EQ(ol->known[15], 9, "battle master knows 9 maneuvers at 15th");
        }
    }
}

static void test_option_spells(void)
{
    int i, j;

    printf("option-dependent spells\n");
    EQ(OPTION_SPELLS_COUNT, 12, "subclass options that grant spells");

    for (i = 0; i < OPTION_SPELLS_COUNT; i++) {
        const OptionSpells *os = &OPTION_SPELLS[i];
        int sub = -1, found_option = 0;
        char buf[512];
        const char *parts[16];
        int n;

        for (j = 0; j < SUBCLASS_COUNT; j++) {
            if (strcmp(SUBCLASSES[j].name, os->subclass) == 0) sub = j;
        }
        if (sub < 0) {
            printf("  FAIL option spells name no subclass: \"%s\"\n",
                   os->subclass);
            failures++;
            continue;
        }

        /* The option must be spelled exactly as the subclass lists it, or
           the match at level-up silently never fires. */
        n = split_pipe(SUBCLASSES[sub].options, buf, sizeof buf, parts, 16);
        for (j = 0; j < n; j++) {
            if (strcmp(parts[j], os->option) == 0) found_option = 1;
        }
        if (!found_option) {
            printf("  FAIL \"%s\" is not an option of %s\n", os->option,
                   os->subclass);
            failures++;
        }

        {
            char sbuf[1024];
            const char *groups[8];
            int ng = split_pipe(os->spells, sbuf, sizeof sbuf, groups, 8);
            int nlevels = 1, k;
            for (k = 0; os->levels[k]; k++) {
                if (os->levels[k] == ',') nlevels++;
            }
            if (ng != nlevels) {
                printf("  FAIL %s has %d spell groups but %d levels\n",
                       os->option, ng, nlevels);
                failures++;
            }
            for (k = 0; k < ng; k++) check_spell_list(groups[k], os->option);
        }
    }
}

static void test_beasts(void)
{
    int i, cr_quarter = 0, moon_cr1 = 0;

    printf("beast stat blocks\n");
    check(BEAST_COUNT_ACTUAL >= 85, "beasts from the Monster Manual",
          BEAST_COUNT_ACTUAL, 85);

    for (i = 0; i < BEAST_COUNT_ACTUAL; i++) {
        const BeastData *b = &BEASTS[i];
        int a;
        if (b->ac < 5 || b->ac > 25) {
            printf("  FAIL %s has AC %d\n", b->name, b->ac);
            failures++;
        }
        if (b->hp < 1 || b->hp > 400) {
            printf("  FAIL %s has %d hit points\n", b->name, b->hp);
            failures++;
        }
        if (!b->speed[0] || !b->cr_text[0]) {
            printf("  FAIL %s is missing its speed or challenge\n", b->name);
            failures++;
        }
        for (a = 0; a < 6; a++) {
            if (b->abilities[a] < 1 || b->abilities[a] > 30) {
                printf("  FAIL %s has an ability score of %d\n", b->name,
                       b->abilities[a]);
                failures++;
            }
        }
        if (i > 0 && strcmp(BEASTS[i - 1].name, b->name) >= 0) {
            printf("  FAIL beasts are not in order at \"%s\"\n", b->name);
            failures++;
        }
        if (b->cr_eighths <= 2) cr_quarter++;
        if (b->cr_eighths <= 8) moon_cr1++;
    }

    /* The stat blocks a druid or ranger reaches for first. */
    {
        int wolf = find_beast("Wolf");
        int bear = find_beast("Brown Bear");
        int eagle = find_beast("Giant Eagle");
        check(wolf >= 0 && bear >= 0 && eagle >= 0,
              "the common beasts are present", 1, 1);
        if (wolf >= 0) {
            EQ(BEASTS[wolf].ac, 13, "wolf armor class");
            EQ(BEASTS[wolf].hp, 11, "wolf hit points");
            EQ(BEASTS[wolf].cr_eighths, 2, "wolf is CR 1/4");
        }
        if (bear >= 0) {
            EQ(BEASTS[bear].hp, 34, "brown bear hit points");
            EQ(BEASTS[bear].cr_eighths, 8, "brown bear is CR 1");
            check(!beast_flies(&BEASTS[bear]), "brown bear cannot fly", 1, 1);
        }
        if (eagle >= 0) {
            check(beast_flies(&BEASTS[eagle]), "giant eagle flies", 1, 1);
            EQ(BEASTS[eagle].abilities[3], 8, "giant eagle Intelligence");
        }
        check(beast_swims(&BEASTS[find_beast("Giant Octopus")]),
              "giant octopus swims", 1, 1);
    }

    /* A 2nd-level druid needs something to turn into. */
    check(cr_quarter > 20, "beasts of CR 1/4 or lower", cr_quarter, 21);
    check(moon_cr1 > 40, "beasts of CR 1 or lower", moon_cr1, 41);
    check(find_beast("No Such Beast") < 0, "unknown beast is not found", 1, 1);
}

static void test_sidekicks(void)
{
    int i, expert = 0, caster = 0, warrior = 0;

    printf("sidekicks\n");

    for (i = 0; i < SIDEKICK_FEATURE_COUNT; i++) {
        const SidekickFeature *f = &SIDEKICK_FEATURES[i];
        if (f->level < 1 || f->level > MAX_LEVEL) {
            printf("  FAIL %s sits at level %d\n", f->name, f->level);
            failures++;
        }
        if (!f->name[0] || !f->summary[0]) {
            printf("  FAIL a sidekick feature is incomplete\n");
            failures++;
        }
        if (f->cls == SK_EXPERT) expert++;
        else if (f->cls == SK_SPELLCASTER) caster++;
        else warrior++;
    }
    check(expert > 0 && caster > 0 && warrior > 0,
          "all three sidekick classes have features", 1, 1);

    /* The Expert's table survived the dump intact, so it is checked exactly:
       six ability score improvements, expertise twice, and the 20th-level
       upgrade to Inspiring Help. */
    {
        int asi = 0, expertise = 0;
        for (i = 0; i < SIDEKICK_FEATURE_COUNT; i++) {
            if (SIDEKICK_FEATURES[i].cls != SK_EXPERT) continue;
            if (!strcmp(SIDEKICK_FEATURES[i].name,
                        "Ability Score Improvement")) asi++;
            if (!strcmp(SIDEKICK_FEATURES[i].name, "Expertise")) expertise++;
        }
        EQ(asi, 6, "Expert ability score improvements");
        EQ(expertise, 2, "Expert gains Expertise twice");
    }

    /* Proficiency follows class level exactly as a character's does. */
    {
        Sidekick sk;
        memset(&sk, 0, sizeof sk);
        for (i = 1; i <= MAX_LEVEL; i++) {
            sk.level = i;
            if (sidekick_proficiency(&sk) != 2 + (i - 1) / 4) {
                printf("  FAIL sidekick proficiency at level %d\n", i);
                failures++;
            }
        }
        sk.abilities[ABL_STR] = 18;
        EQ(sidekick_ability_mod(&sk, ABL_STR), 4, "sidekick ability modifier");
    }

    /* Cantrips known is the one Spellcaster column that could be read. */
    EQ(SPELLCASTER_CANTRIPS[1], 2, "Spellcaster cantrips at 1st level");
    EQ(SPELLCASTER_CANTRIPS[4], 3, "Spellcaster cantrips at 4th level");
    EQ(SPELLCASTER_CANTRIPS[10], 4, "Spellcaster cantrips at 10th level");
    EQ(SPELLCASTER_SPELLS_KNOWN[1], 1, "Spellcaster knows one spell at 1st");

    /* Whatever the reconstruction says, it must never go backwards. */
    for (i = 2; i <= MAX_LEVEL; i++) {
        if (SPELLCASTER_SPELLS_KNOWN[i] < SPELLCASTER_SPELLS_KNOWN[i - 1]
            || SPELLCASTER_CANTRIPS[i] < SPELLCASTER_CANTRIPS[i - 1]) {
            printf("  FAIL Spellcaster forgets something at level %d\n", i);
            failures++;
        }
    }

    /* A sidekick must be buildable: there have to be creatures that qualify. */
    {
        int eligible = 0;
        for (i = 0; i < BEAST_COUNT_ACTUAL; i++) {
            if (BEASTS[i].cr_eighths <= 4) eligible++;
        }
        check(eligible > 30, "beasts of CR 1/2 or lower for a sidekick",
              eligible, 31);
    }
}

static void test_homebrew_banks(void)
{
    printf("homebrew banks\n");

    /* With no homebrew loaded, each bank must be exactly what the books
       give. If these ever drift, something has repointed a bank without
       going through homebrew.c. */
    check(ITEMS == BOOK_ITEMS, "item bank starts as the book's", 1, 1);
    check(SPELLS == BOOK_SPELLS, "spell bank starts as the book's", 1, 1);
    check(MAGIC_ITEMS == BOOK_MAGIC_ITEMS,
          "magic item bank starts as the book's", 1, 1);
    EQ(ITEM_COUNT, BOOK_ITEM_COUNT, "item count");
    EQ(SPELL_COUNT, BOOK_SPELL_COUNT, "spell count");
    EQ(MAGIC_ITEM_COUNT, BOOK_MAGIC_ITEM_COUNT, "magic item count");

    /* Homebrew is a source book like any other, so it can be switched off. */
    EQ(BOOK_HOMEBREW, BOOK_COUNT - 1, "homebrew is the last source");
    check(book_enabled(BOOK_HOMEBREW), "homebrew is on by default", 1, 1);
    {
        Settings s;
        settings_defaults(&s);
        EQ(s.book[BOOK_HOMEBREW], 1, "homebrew defaults on");
    }
    check(BOOK_NAME[BOOK_HOMEBREW] != NULL && BOOK_NAME[BOOK_HOMEBREW][0],
          "homebrew has a name", 1, 1);
    check(BOOK_ABBREV[BOOK_HOMEBREW] != NULL
          && BOOK_ABBREV[BOOK_HOMEBREW][0], "homebrew has an abbreviation",
          1, 1);

    /* Nothing in the books should claim to be homebrew. */
    {
        int i, stray = 0;
        for (i = 0; i < BOOK_ITEM_COUNT; i++) {
            if (BOOK_ITEMS[i].book == BOOK_HOMEBREW) stray++;
        }
        for (i = 0; i < BOOK_MAGIC_ITEM_COUNT; i++) {
            if (BOOK_MAGIC_ITEMS[i].book == BOOK_HOMEBREW) stray++;
        }
        for (i = 0; i < BOOK_SPELL_COUNT; i++) {
            if (BOOK_SPELLS[i].book == BOOK_HOMEBREW) stray++;
        }
        EQ(stray, 0, "no printed entry is tagged homebrew");
    }
}

/* Adds a magic item by name and returns whether it landed. */
static int give_magic(Character *c, const char *name, int attuned, int plus,
                      int equipped)
{
    int id = find_magic_item(name);
    if (id < 0) return 0;
    add_magic_item(c, id, 1, attuned, plus);
    c->inventory[c->item_count - 1].equipped = equipped;
    return 1;
}

static void test_magic_armour_class(void)
{
    Character c;

    printf("magic items in Armor Class\n");

    /* Every rule must name a magic item that exists, or it silently never
       fires. */
    {
        int i;
        for (i = 0; i < MAGIC_RULE_COUNT; i++) {
            if (find_magic_item(MAGIC_RULES[i].item) < 0) {
                printf("  FAIL magic rule names no item: \"%s\"\n",
                       MAGIC_RULES[i].item);
                failures++;
            }
        }
    }

    /* A ring of protection is +1 AC and +1 to every save -- but only once
       attuned. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    c.base_score[ABL_DEX] = 10;
    c.base_score[ABL_CON] = 10;
    c.base_score[ABL_STR] = 10;
    EQ(armour_class(&c), 10, "unarmoured, no magic");
    check(give_magic(&c, "Ring of Protection", 0, 0, 0),
          "ring of protection exists", 1, 1);
    EQ(armour_class(&c), 10, "unattuned ring does nothing");
    c.inventory[c.item_count - 1].attuned = 1;
    EQ(armour_class(&c), 11, "attuned ring is +1 AC");
    EQ(save_bonus(&c, ABL_STR), 1, "attuned ring is +1 to saves");

    /* Bracers of Defense are +2, but only with no armour and no shield. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    c.base_score[ABL_DEX] = 10;
    give_magic(&c, "Bracers of Defense", 1, 0, 0);
    EQ(armour_class(&c), 12, "bracers of defense unarmoured");
    add_item_by_name(&c, "Chain mail", 1, 1);
    EQ(armour_class(&c), 16, "bracers do nothing in armour");

    /* Magic armour replaces the base, and only counts once worn. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    c.base_score[ABL_DEX] = 10;
    give_magic(&c, "Dwarven Plate", 0, 0, 0);
    EQ(armour_class(&c), 10, "dwarven plate carried, not worn");
    c.inventory[c.item_count - 1].equipped = 1;
    EQ(armour_class(&c), 20, "dwarven plate worn is AC 20");

    /* Elven chain caps Dexterity at +2 like the chain shirt it is. */
    reset(&c);
    add_class(&c, CLS_ROGUE, 1, -1);
    c.base_score[ABL_DEX] = 20;                   /* +5, capped to +2 */
    give_magic(&c, "Elven Chain", 0, 0, 1);
    EQ(armour_class(&c), 16, "elven chain with high Dexterity");

    /* A +2 shield is the shield's own +2 plus the enchantment. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    c.base_score[ABL_DEX] = 10;
    give_magic(&c, "Shield, +1, +2, or +3", 0, 2, 1);
    EQ(armour_class(&c), 14, "a +2 shield is +4 in total");

    /* A sentinel shield is an ordinary shield's +2 and nothing more; what
       it really does -- advantage on initiative and Perception -- is not a
       number the sheet can carry. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    c.base_score[ABL_DEX] = 10;
    give_magic(&c, "Sentinel Shield", 0, 0, 1);
    EQ(armour_class(&c), 12, "a sentinel shield is +2");

    /* A luckstone is +1 to saves, once attuned. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    c.base_score[ABL_CON] = 10;
    give_magic(&c, "Stone of Good Luck (Luckstone)", 1, 0, 0);
    EQ(save_bonus(&c, ABL_CON), 1, "a luckstone is +1 to saves");

    /* Bonuses stack: magic armour, a magic shield and a worn ring. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    c.base_score[ABL_DEX] = 10;
    give_magic(&c, "Dwarven Plate", 0, 0, 1);
    give_magic(&c, "Shield, +1, +2, or +3", 0, 1, 1);
    give_magic(&c, "Cloak of Protection", 1, 0, 0);
    c.base_score[ABL_WIS] = 10;
    EQ(armour_class(&c), 24, "plate 20, shield +3, cloak +1");
    EQ(save_bonus(&c, ABL_WIS), 1, "cloak raises saves too");

    /* Magic armour still switches off a barbarian's Unarmored Defense. */
    reset(&c);
    add_class(&c, CLS_BARBARIAN, 1, -1);
    c.base_score[ABL_DEX] = 16;                   /* +3 */
    c.base_score[ABL_CON] = 16;                   /* +3 */
    EQ(armour_class(&c), 16, "barbarian unarmoured");
    give_magic(&c, "Dwarven Plate", 0, 0, 1);
    EQ(armour_class(&c), 20, "plate replaces Unarmored Defense");

    /* The robe sets the unarmoured base rather than adding to it. */
    reset(&c);
    add_class(&c, CLS_WIZARD, 1, -1);
    c.base_score[ABL_DEX] = 14;                   /* +2 */
    give_magic(&c, "Robe of the Archmagi", 1, 0, 0);
    EQ(armour_class(&c), 17, "robe of the archmagi is 15 + Dexterity");
}

static void test_magic_scores_and_speeds(void)
{
    Character c;
    char buf[256];

    printf("magic items in scores, speeds and defences\n");

    /* An amulet of health sets Constitution to 19, and only helps if the
       score is not already higher. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    c.base_score[ABL_CON] = 12;
    EQ(ability_score(&c, ABL_CON), 12, "constitution before the amulet");
    give_magic(&c, "Amulet of Health", 1, 0, 0);
    EQ(ability_score(&c, ABL_CON), 19, "amulet of health sets it to 19");
    c.base_score[ABL_CON] = 20;
    EQ(ability_score(&c, ABL_CON), 20, "a higher score is left alone");

    /* Unattuned, it does nothing at all. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    c.base_score[ABL_INT] = 8;
    give_magic(&c, "Headband of Intellect", 0, 0, 0);
    EQ(ability_score(&c, ABL_INT), 8, "unattuned headband does nothing");

    /* The Hand of Vecna sets Strength to 20, the same way. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    c.base_score[ABL_STR] = 13;
    give_magic(&c, "Hand of Vecna", 1, 0, 0);
    EQ(ability_score(&c, ABL_STR), 20, "the hand of Vecna sets Strength to 20");

    /* A belt of giant strength carries the score of its own giant. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    c.base_score[ABL_STR] = 10;
    give_magic(&c, "Belt of Giant Strength", 1, 27, 0);
    EQ(ability_score(&c, ABL_STR), 27, "cloud giant belt");
    EQ(carrying_capacity(&c), 27 * 15, "and it changes what you can carry");

    /* Boots of striding and springing set a floor under the walking speed. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    c.race_id = 0;
    {
        int bare = speed_of(&c);
        give_magic(&c, "Boots of Striding and Springing", 1, 0, 0);
        check(speed_of(&c) >= 30 && speed_of(&c) >= bare,
              "boots set a floor of 30 feet", speed_of(&c), 30);
    }

    /* Movement a magic item grants shows up separately. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    EQ(magic_fly_speed(&c), 0, "no flying without an item");
    give_magic(&c, "Wings of Flying", 1, 0, 0);
    EQ(magic_fly_speed(&c), 60, "wings of flying");
    give_magic(&c, "Ring of Swimming", 0, 0, 0);
    EQ(magic_swim_speed(&c), 40, "ring of swimming needs no attunement");

    /* Winged boots grant a flying speed equal to the walking speed. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    give_magic(&c, "Winged Boots", 1, 0, 0);
    EQ(magic_fly_speed(&c), speed_of(&c), "winged boots match your speed");

    /* Resistances are collected, including the one the copy names. */
    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    EQ(magic_defences(&c, buf, sizeof buf), 0, "nothing granted yet");
    give_magic(&c, "Brooch of Shielding", 1, 0, 0);
    EQ(magic_defences(&c, buf, sizeof buf), 1, "brooch grants one");
    check(strstr(buf, "force") != NULL, "and it is force damage", 1, 1);

    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    give_magic(&c, "Ring of Resistance", 1, 0, 0);
    snprintf(c.inventory[c.item_count - 1].variant,
             sizeof c.inventory[0].variant, "%s", "lightning");
    magic_defences(&c, buf, sizeof buf);
    check(strstr(buf, "lightning") != NULL,
          "a ring of resistance names its own damage", 1, 1);

    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    give_magic(&c, "Periapt of Proof against Poison", 0, 0, 0);
    magic_defences(&c, buf, sizeof buf);
    check(strstr(buf, "immune to poison") != NULL, "immunity reads as one",
          1, 1);
}

static void test_life_tables(void)
{
    int i, j;

    printf("backstory tables\n");
    check(LIFE_TABLE_COUNT >= 10, "tables read from Xanathar's",
          LIFE_TABLE_COUNT, 10);

    for (i = 0; i < LIFE_TABLE_COUNT; i++) {
        const LifeTable *t = &LIFE_TABLES[i];
        int n = 1, sides = 20, lo, hi, v;
        const char *d = strchr(t->die, 'd');

        if (!t->name[0] || !t->die[0] || t->count <= 0) {
            printf("  FAIL a backstory table is incomplete\n");
            failures++;
            continue;
        }
        if (d && d != t->die) n = t->die[0] - '0';
        if (d) sides = atoi(d + 1);
        lo = n;
        hi = n * sides;

        /* Every result the die can give must land on exactly one row --
           a gap would leave a roll with no answer, an overlap would make
           the table ambiguous. */
        for (v = lo; v <= hi; v++) {
            int hits = 0;
            for (j = 0; j < t->count; j++) {
                if (v >= t->rows[j].lo && v <= t->rows[j].hi) hits++;
            }
            if (hits != 1) {
                printf("  FAIL %s: rolling %d hits %d rows\n", t->name, v,
                       hits);
                failures++;
                break;
            }
        }

        /* Rows should read as text, not as leftover OCR marks. */
        for (j = 0; j < t->count; j++) {
            const char *txt = t->rows[j].text;
            size_t len = strlen(txt);
            if (len < 2 || len > 200) {
                printf("  FAIL %s has a row of %d characters\n", t->name,
                       (int)len);
                failures++;
                break;
            }
            if (strstr(txt, "  ") || txt[len - 1] == '-') {
                printf("  FAIL %s row reads badly: \"%s\"\n", t->name, txt);
                failures++;
                break;
            }
        }
    }
}

static void test_notes_and_custom_background(void)
{
    Character c;
    int i;

    printf("notes and a background of your own\n");

    reset(&c);
    EQ(c.note_count, 0, "a new character has no notes");
    EQ(c.background_id, -1, "a fresh character has no background yet");

    /* Notes hold what they are given, up to the limit. */
    for (i = 0; i < MAX_NOTES + 4; i++) {
        if (c.note_count >= MAX_NOTES) break;
        snprintf(c.notes[c.note_count].title, MAX_NAME, "note %d", i);
        snprintf(c.notes[c.note_count].body, MAX_LORE, "body %d", i);
        c.note_count++;
    }
    EQ(c.note_count, MAX_NOTES, "notes fill to the limit and stop");
    check(strcmp(c.notes[0].title, "note 0") == 0, "the first note is kept",
          1, 1);
    check(strcmp(c.notes[MAX_NOTES - 1].title, "note 15") == 0,
          "and so is the last", 1, 1);

    /* A note's body holds far more than the other text on a character, so
       lore fits without being cut short. */
    check(MAX_LORE >= 2048, "a note holds paragraphs, not a line",
          MAX_LORE, 2048);
    {
        char big[MAX_LORE];
        memset(big, 'x', sizeof big - 1);
        big[sizeof big - 1] = '\0';
        snprintf(c.notes[0].body, MAX_LORE, "%s", big);
        EQ((int)strlen(c.notes[0].body), MAX_LORE - 1,
           "and it keeps all of it");
    }
    {
        /* Newlines survive, because a note is written as paragraphs. */
        snprintf(c.notes[1].body, MAX_LORE, "first\nsecond\n\nfourth");
        check(strchr(c.notes[1].body, '\n') != NULL,
              "paragraph breaks are kept", 1, 1);
    }

    /* Removing one closes the gap, as the screen does. */
    {
        int k;
        for (k = 0; k < c.note_count - 1; k++) {
            c.notes[k] = c.notes[k + 1];
        }
        c.note_count--;
        EQ(c.note_count, MAX_NOTES - 1, "removing a note shortens the list");
        check(strcmp(c.notes[0].title, "note 1") == 0,
              "and the rest move up", 1, 1);
    }

    /* A background of your own is marked by an id of -1 and carries its own
       name and feature, so nothing indexes the table. */
    reset(&c);
    c.background_id = -1;
    snprintf(c.background_name, sizeof c.background_name, "Sky Pilgrim");
    snprintf(c.background_feature, sizeof c.background_feature,
             "Reader of Winds");
    check(c.background_id < 0, "a custom background has no table row", 1, 1);
    check(c.background_name[0] != '\0', "but it does have a name", 1, 1);

    /* The two skills it grants are ordinary skill proficiencies. */
    c.skill_prof[SKL_SURVIVAL] = 1;
    c.skill_prof[SKL_INSIGHT] = 1;
    EQ(skill_bonus(&c, SKL_SURVIVAL) - ability_mod(&c, ABL_WIS),
       proficiency_bonus(&c), "a custom background's skill is proficient");
}

/* The lists the builder offers, and the room it leaves beside them. */
/* The attacks block: what a weapon hits at and what it does. */
static void test_attacks(void)
{
    Character c;
    Attack a[MAX_ATTACKS];
    int n, i;

    printf("attacks\n");

    /* A mountain dwarf fighter, as in chapter 1: Strength 17, proficiency
       +2, proficient with martial weapons. */
    reset(&c);
    c.race_id = find_race("Dwarf");
    c.subrace_id = find_subrace("Mountain Dwarf");
    add_class(&c, CLS_FIGHTER, 1, -1);
    c.base_score[ABL_STR] = 15;
    c.base_score[ABL_DEX] = 10;
    c.racial_bonus[ABL_STR] = 2;
    add_prof(&c, "Simple weapons");
    add_prof(&c, "Martial weapons");
    add_item_by_name(&c, "Battleaxe", 1, 0);
    add_item_by_name(&c, "Light crossbow", 1, 0);   /* a simple weapon */

    n = attacks_of(&c, a, MAX_ATTACKS);
    EQ(n, 3, "two weapons and an unarmed strike");
    for (i = 0; i < n; i++) {
        if (!strcmp(a[i].name, "Battleaxe")) {
            EQ(a[i].bonus, 5, "battleaxe to hit (Str 3 + prof 2)");
            check(!strcmp(a[i].damage, "1d8+3 slashing"),
                  "battleaxe damage", 0, 0);
        } else if (!strcmp(a[i].name, "Light crossbow")) {
            EQ(a[i].bonus, 2, "light crossbow to hit (Dex 0 + prof 2)");
        } else if (!strcmp(a[i].name, "Unarmed strike")) {
            check(!strcmp(a[i].damage, "4 bludgeoning"),
                  "unarmed strike damage (1 + Str 3)", 0, 0);
        }
    }

    /* Finesse takes the better of Strength and Dexterity, and a weapon the
       character has no proficiency with says so. */
    reset(&c);
    c.race_id = find_race("Human");
    add_class(&c, CLS_ROGUE, 1, -1);
    c.base_score[ABL_STR] = 8;
    c.base_score[ABL_DEX] = 16;
    add_prof(&c, "Simple weapons");
    add_item_by_name(&c, "Dagger", 1, 0);       /* finesse, simple */
    add_item_by_name(&c, "Greataxe", 1, 0);     /* martial, no proficiency */

    n = attacks_of(&c, a, MAX_ATTACKS);
    for (i = 0; i < n; i++) {
        if (!strcmp(a[i].name, "Dagger")) {
            EQ(a[i].bonus, 5, "dagger to hit (finesse: Dex 3 + prof 2)");
            check(!strcmp(a[i].damage, "1d4+3 piercing"),
                  "dagger damage takes Dexterity", 0, 0);
        } else if (!strcmp(a[i].name, "Greataxe")) {
            EQ(a[i].bonus, -1, "greataxe to hit (Str -1, no proficiency)");
            EQ(a[i].proficient, 0, "not proficient with a greataxe");
        }
    }

    /* A monk's unarmed strike uses the Martial Arts die, and Dexterity
       when it is the better modifier. */
    reset(&c);
    c.race_id = find_race("Human");
    add_class(&c, CLS_MONK, 5, -1);
    c.base_score[ABL_STR] = 10;
    c.base_score[ABL_DEX] = 18;
    n = attacks_of(&c, a, MAX_ATTACKS);
    for (i = 0; i < n; i++) {
        if (!strcmp(a[i].name, "Unarmed strike")) {
            EQ(a[i].bonus, 7, "monk unarmed to hit (Dex 4 + prof 3)");
            check(!strcmp(a[i].damage, "1d6+4 bludgeoning"),
                  "5th-level Martial Arts die", 0, 0);
        }
    }

    /* The experience table, and the height and weight rows. */
    EQ(XP_FOR_LEVEL[1], 0, "level 1 costs nothing");
    EQ(XP_FOR_LEVEL[5], 6500, "level 5 experience");
    EQ(XP_FOR_LEVEL[20], 355000, "level 20 experience");
    for (i = 2; i <= MAX_LEVEL; i++) {
        if (XP_FOR_LEVEL[i] <= XP_FOR_LEVEL[i - 1]) {
            printf("  FAIL experience does not rise at level %d\n", i);
            failures++;
        }
    }

    check(body_for("Dwarf", "Hill Dwarf") != NULL,
          "the height and weight table covers the hill dwarf", 1, 1);
    check(body_for("Halfling", NULL) != NULL,
          "and a race with no subrace row", 1, 1);
    check(body_for("Aarakocra", NULL) == NULL,
          "and not a race the table never listed", 1, 1);
    for (i = 0; i < BODY_COUNT; i++) {
        if (find_race(BODIES[i].race) < 0) {
            printf("  FAIL height and weight row names no race: \"%s\"\n",
                   BODIES[i].race);
            failures++;
        }
    }

    EQ(CONDITION_COUNT, 15, "conditions, appendix A plus exhaustion");

    /* The wizard builds each menu into an array of MENU_MAX entries. A
       table that outgrows it used to be written past the end of that array:
       42 races against 32 slots put the last ten races into the details of
       the first ten, and would have corrupted the stack on a longer list. */
    EQ(RACE_COUNT <= MENU_MAX, 1, "races fit the race menu");
    EQ(SUBRACE_COUNT <= MENU_MAX, 1, "subraces fit");
    EQ(CLASS_COUNT <= MENU_MAX, 1, "classes fit");
    EQ(BACKGROUND_COUNT + 1 <= MENU_MAX,
       1, "backgrounds fit, with a slot kept for one of your own");
}

static void test_choice_lists(void)
{
    const char *tools[64];
    int i, j, n;

    printf("tool groups and feat choices\n");

    n = tools_in_group("Artisan's tools", tools, 64);
    check(n == 17, "artisan's tools", n, 17);
    n = tools_in_group("Musical instrument", tools, 64);
    check(n == 10, "musical instruments", n, 10);
    n = tools_in_group("Gaming set", tools, 64);
    check(n == 2, "gaming sets", n, 2);

    /* A group nobody has heard of yields every tool, so a proficiency the
       books word some other way still gets a list rather than a blank. */
    n = tools_in_group("Siege engines", tools, 64);
    check(n > 30, "an unknown group falls back to every tool", n, 31);

    for (i = 0; i < TOOL_GROUP_COUNT; i++) {
        int id = find_item(TOOL_GROUPS[i].item);
        if (id < 0) {
            printf("  FAIL tool group names no item: \"%s\"\n",
                   TOOL_GROUPS[i].item);
            failures++;
        } else if (ITEMS[id].category != ITEM_TOOL) {
            printf("  FAIL \"%s\" is in a tool group but is not a tool\n",
                   TOOL_GROUPS[i].item);
            failures++;
        }
    }

    /* Every tool a background or class can be asked to choose has to come
       from a group the builder knows how to offer. */
    for (i = 0; i < BACKGROUND_COUNT; i++) {
        const char *t = BACKGROUNDS[i].tool_profs;
        if (!contains_ci(t, "one type of")) continue;
        if (contains_ci(t, "artisan") || contains_ci(t, "gaming set")
            || contains_ci(t, "musical instrument")) continue;
        printf("  FAIL %s grants an open tool choice with no group: \"%s\"\n",
               BACKGROUNDS[i].name, t);
        failures++;
    }

    /* Each feat the builder asks a follow-up question for must still exist,
       since the questions are keyed on the printed name. */
    for (i = 0; i < FEATS_WITH_CHOICES_COUNT; i++) {
        int found = 0;
        for (j = 0; j < FEAT_COUNT; j++) {
            if (strcmp(FEATS[j].name, FEATS_WITH_CHOICES[i]) == 0) found = 1;
        }
        if (!found) {
            printf("  FAIL no feat named \"%s\"\n", FEATS_WITH_CHOICES[i]);
            failures++;
        }
    }

    /* The two lists a feat can draw on are found by their plural name. */
    {
        int inv = 0, meta = 0;
        for (i = 0; i < OPTION_LIST_COUNT; i++) {
            if (!strcmp(OPTION_LISTS[i].plural, "eldritch invocations")) inv = 1;
            if (!strcmp(OPTION_LISTS[i].plural, "metamagic options")) meta = 1;
        }
        check(inv, "Eldritch Adept can find the invocation list", inv, 1);
        check(meta, "Metamagic Adept can find the metamagic list", meta, 1);
    }

    /* Eldritch Adept on a non-warlock is limited to invocations with no
       prerequisite at all; there have to be some. */
    for (i = 0; i < OPTION_LIST_COUNT; i++) {
        const OptionList *ol = &OPTION_LISTS[i];
        int open = 0;
        if (strcmp(ol->plural, "eldritch invocations") != 0) continue;
        for (j = 0; j < ol->count; j++) {
            if (!ol->options[j].prereq[0] && ol->options[j].min_level == 0)
                open++;
        }
        check(open >= 8, "invocations open to Eldritch Adept", open, 8);
    }
}

static void test_carrying_and_coins(void)
{
    Character c;

    printf("weight and coins\n");
    reset(&c);
    add_class(&c, CLS_FIGHTER, 1, -1);
    c.base_score[ABL_STR] = 15;
    EQ(carrying_capacity(&c), 225, "Str 15 x 15");

    add_item_by_name(&c, "Chain mail", 1, 1);     /* 55 lb */
    EQ(current_weight_tenths(&c), 550, "chain mail weighs 55 lb");

    c.gold = 100;                                  /* 50 coins to the pound */
    EQ(current_weight_tenths(&c), 570, "100 gp adds 2 lb");
}


/* ----------------------------------------------- the whole data matrix */

/* Round trips a character through the file and back, and checks the file it
   writes the second time is the file it wrote the first.
 *
 * tools/roundtrip.py makes this check on characters the wizard happens to
 * build; here it runs over every row of every table, which is how a race,
 * a subclass, a magic item or a spell that the writer and the reader
 * disagree about gets found rather than waited for. */
static char sheet_a[600000], sheet_b[600000];

static size_t slurp(const char *path, char *out, size_t max)
{
    FILE *f = fopen(path, "r");
    size_t n;

    if (!f) return 0;
    n = fread(out, 1, max - 1, f);
    out[n] = '\0';
    fclose(f);
    return n;
}

/* Returns 1 when the character survives being written and read back. */
static int survives_the_file(const Character *c, const char *what)
{
    char path[MAX_NAME + 8];
    Character back;
    size_t na, nb;

    if (save_character(c, path, sizeof path) != 0) {
        printf("  FAIL %-52s could not be written\n", what);
        failures++;
        return 0;
    }
    na = slurp(path, sheet_a, sizeof sheet_a);
    if (load_character(path, &back) != 0) {
        printf("  FAIL %-52s could not be read back\n", what);
        failures++;
        remove(path);
        return 0;
    }
    if (save_character(&back, path, sizeof path) != 0) {
        printf("  FAIL %-52s could not be rewritten\n", what);
        failures++;
        remove(path);
        return 0;
    }
    nb = slurp(path, sheet_b, sizeof sheet_b);
    remove(path);

    if (na == 0 || na != nb || memcmp(sheet_a, sheet_b, na) != 0) {
        size_t i = 0, line = 1, col = 0;
        while (i < na && i < nb && sheet_a[i] == sheet_b[i]) {
            if (sheet_a[i] == '\n') { line++; col = 0; } else col++;
            i++;
        }
        printf("  FAIL %-52s differs at line %lu column %lu\n",
               what, (unsigned long)line, (unsigned long)col);
        printf("        wrote %.60s\n", sheet_a + i - col);
        printf("        read  %.60s\n", sheet_b + i - col);
        failures++;
        return 0;
    }
    return 1;
}

static void base_character(Character *c, const char *name)
{
    int a;

    reset(c);
    snprintf(c->name, sizeof c->name, "%s", name);
    snprintf(c->player, sizeof c->player, "Self Test");
    for (a = 0; a < ABL_COUNT; a++) c->base_score[a] = 12;
}

/* The sweep's usual subject: a 1st-level fighter of the first race, with one
   hit die rolled, ready to have the row under test hung on it. */
static void plain_fighter(Character *c, const char *name)
{
    base_character(c, name);
    c->race_id = 0;
    add_class(c, CLS_FIGHTER, 1, -1);
    c->hp_rolls[0] = 10;
    c->hp_roll_count = 1;
}

/* Every race and subrace, every class at every level, and every subclass:
   built, written, read back and written again. */
static void test_sweep_characters(void)
{
    Character c;
    int i, lvl;

    printf("every race, class and subclass through the file\n");

    for (i = 0; i < RACE_COUNT; i++) {
        int k, subs = RACES[i].subrace_count;

        base_character(&c, "SelftestRace");
        c.race_id = i;
        if (RACES[i].has_ancestry) c.ancestry_id = 0;
        add_class(&c, CLS_FIGHTER, 1, -1);
        c.hp_rolls[0] = 10;
        c.hp_roll_count = 1;
        if (!survives_the_file(&c, RACES[i].name)) return;

        for (k = 0; k < subs; k++) {
            c.subrace_id = RACES[i].first_subrace + k;
            if (!survives_the_file(&c, SUBRACES[c.subrace_id].name)) return;
        }
    }

    for (i = 0; i < CLASS_COUNT; i++) {
        for (lvl = 1; lvl <= MAX_LEVEL; lvl++) {
            int h;

            base_character(&c, "SelftestClass");
            c.race_id = 0;
            add_class(&c, i, lvl, -1);
            for (h = 0; h < lvl; h++) c.hp_rolls[h] = 5;
            c.hp_roll_count = lvl;
            if (!survives_the_file(&c, CLASSES[i].name)) return;
        }
    }

    for (i = 0; i < SUBCLASS_COUNT; i++) {
        int h;

        base_character(&c, "SelftestSubclass");
        c.race_id = 0;
        add_class(&c, SUBCLASSES[i].class_id, MAX_LEVEL, i);
        for (h = 0; h < MAX_LEVEL; h++) c.hp_rolls[h] = 5;
        c.hp_roll_count = MAX_LEVEL;
        if (!survives_the_file(&c, SUBCLASSES[i].name)) return;
    }
}

/* Every spell, every magic item and every beast, one at a time. A row the
   sheet cannot print, or that the loader cannot find its way back to, shows
   up here rather than on the day a player picks it. */
static void test_sweep_content(void)
{
    Character c;
    int i;

    printf("every spell, magic item and beast through the file\n");

    for (i = 0; i < SPELL_COUNT; i++) {
        base_character(&c, "SelftestSpell");
        c.race_id = 0;
        add_class(&c, CLS_WIZARD, MAX_LEVEL, -1);
        c.hp_rolls[0] = 6;
        c.hp_roll_count = 1;
        c.spells[0].spell_id = i;
        c.spells[0].class_id = CLS_WIZARD;
        c.spells[0].prepared = 1;
        c.spell_count = 1;
        if (!survives_the_file(&c, SPELLS[i].name)) return;
    }

    for (i = 0; i < MAGIC_ITEM_COUNT; i++) {
        plain_fighter(&c, "SelftestMagic");
        add_magic_item(&c, i, 1, MAGIC_ITEMS[i].attunement ? 1 : 0, 0);
        if (!survives_the_file(&c, MAGIC_ITEMS[i].name)) return;
    }

    for (i = 0; i < ITEM_COUNT; i++) {
        plain_fighter(&c, "SelftestItem");
        add_item(&c, i, 1, 0);
        if (!survives_the_file(&c, ITEMS[i].name)) return;
    }

    for (i = 0; i < BEAST_COUNT_ACTUAL; i++) {
        Sidekick *sk;

        plain_fighter(&c, "SelftestBeast");
        sk = &c.sidekicks[c.sidekick_count++];
        memset(sk, 0, sizeof *sk);
        snprintf(sk->name, sizeof sk->name, "Companion");
        snprintf(sk->creature, sizeof sk->creature, "%s", BEASTS[i].name);
        snprintf(sk->speed, sizeof sk->speed, "%s", BEASTS[i].speed);
        sk->cls = SK_WARRIOR;
        sk->level = 1;
        sk->role = -1;
        sk->hp = BEASTS[i].hp;
        sk->ac = BEASTS[i].ac;
        if (!survives_the_file(&c, BEASTS[i].name)) return;
    }
}

/* Text that the '|' separated format cannot take at face value. A name, a
   note or a tool with a separator or an escape in it used to be written
   raw, and everything after the stray character was lost on reload. */
static void test_awkward_text(void)
{
    static const char *const AWKWARD[] = {
        "a|b", "back\\slash", "both|and\\here", "#END-DNDDATA",
        "#BEGIN-DNDDATA v1", "NAME|spoof", "trailing|", "|leading",
        "\\p literal", "\\n literal", "pipes||together"
    };
    const int n = (int)(sizeof AWKWARD / sizeof AWKWARD[0]);
    Character c, back;
    char path[MAX_NAME + 8];
    int i;

    printf("text the file format has to escape\n");

    for (i = 0; i < n; i++) {
        plain_fighter(&c, "SelftestText");

        snprintf(c.player, sizeof c.player, "%s", AWKWARD[i]);
        snprintf(c.trait, sizeof c.trait, "%s", AWKWARD[i]);
        snprintf(c.appearance, sizeof c.appearance, "%s", AWKWARD[i]);
        snprintf(c.eyes, sizeof c.eyes, "%s", AWKWARD[i]);
        add_language(&c, AWKWARD[i]);
        add_tool(&c, AWKWARD[i]);
        add_choice(&c, "Fighting Style", AWKWARD[i]);
        snprintf(c.notes[0].title, sizeof c.notes[0].title, "%s", AWKWARD[i]);
        snprintf(c.notes[0].body, sizeof c.notes[0].body,
                 "%s\nsecond line\n%s", AWKWARD[i], AWKWARD[i]);
        c.note_count = 1;

        if (save_character(&c, path, sizeof path) != 0
            || load_character(path, &back) != 0) {
            printf("  FAIL %-52s would not save and load\n", AWKWARD[i]);
            failures++;
            remove(path);
            continue;
        }
        remove(path);

        check(!strcmp(back.player, c.player), "player survives", 0, 0);
        check(!strcmp(back.trait, c.trait), "trait survives", 0, 0);
        check(!strcmp(back.appearance, c.appearance), "appearance survives",
              0, 0);
        check(!strcmp(back.eyes, c.eyes), "eye colour survives", 0, 0);
        check(back.language_count == c.language_count
              && !strcmp(back.languages[0], c.languages[0]),
              "language survives", 0, 0);
        check(back.tool_prof_count == c.tool_prof_count
              && !strcmp(back.tool_profs[0], c.tool_profs[0]),
              "tool survives", 0, 0);
        check(back.choice_count == c.choice_count
              && !strcmp(back.choices[0].value, c.choices[0].value),
              "choice survives", 0, 0);
        check(back.note_count == 1
              && !strcmp(back.notes[0].title, c.notes[0].title)
              && !strcmp(back.notes[0].body, c.notes[0].body),
              "note survives", 0, 0);
    }

    /* A note long enough to have been cut in half by the read buffer. */
    plain_fighter(&c, "SelftestLongNote");
    for (i = 0; i + 8 < (int)sizeof c.notes[0].body - 1; i += 8) {
        memcpy(c.notes[0].body + i, "abcdefg\n", 8);
    }
    c.notes[0].body[i] = '\0';
    snprintf(c.notes[0].title, sizeof c.notes[0].title, "A very long note");
    c.note_count = 1;
    if (save_character(&c, path, sizeof path) == 0
        && load_character(path, &back) == 0) {
        EQ((long)strlen(back.notes[0].body), (long)strlen(c.notes[0].body),
           "a note of 2000 characters comes back whole");
    } else {
        printf("  FAIL long note would not save and load\n");
        failures++;
    }
    remove(path);
}

/* The two id spaces, and a row the loader must decline. */
static void test_inventory_id_spaces(void)
{
    Character c;

    printf("the two item tables are told apart\n");

    base_character(&c, "SelftestIds");
    c.race_id = 0;
    add_class(&c, CLS_FIGHTER, 1, -1);

    /* item_id indexes MAGIC_ITEMS here and ITEMS there; an ordinary item
       must never be merged into a magic entry that shares its index. */
    add_magic_item(&c, 5, 1, 0, 0);
    add_item(&c, 5, 1, 0);
    EQ(c.item_count, 2, "an item and a magic item at index 5 stay apart");
    EQ(c.inventory[0].quantity, 1, "the magic item keeps its quantity");
    EQ(c.inventory[0].is_magic, 1, "the first entry is the magic one");
    EQ(c.inventory[1].is_magic, 0, "the second is the ordinary one");

    /* A quantity of zero is declined, and nothing may be written for it. */
    base_character(&c, "SelftestZero");
    add_magic_item(&c, 5, 0, 0, 0);
    EQ(c.item_count, 0, "a quantity of zero adds nothing");
}


/* homebrew.txt is the one file read while the program runs, and a DM types
   its contents. It shares the character file's record format, so a name with
   a separator in it has to survive being written and read the same way. */
static void test_homebrew_file(void)
{
    static char before[200000], after[200000];
    size_t n_before;
    FILE *f;
    int i, found;

    printf("the homebrew file keeps what the DM typed\n");

    /* The DM's own file is put back exactly as it was found. */
    n_before = slurp("homebrew.txt", before, sizeof before);

    f = fopen("homebrew.txt", "w");
    if (!f) {
        printf("  FAIL could not write a homebrew file to test with\n");
        failures++;
        return;
    }
    fputs("MAGICITEM|Hammer\\pof Dawn|wondrous item|rare|"
          "requires attunement|It shines a\\pbit, and costs 5\\\\10 gp.\n", f);
    fputs("ITEM|Pole\\parm|martial-melee|2000|60|0|0|0|0|1d10|slashing|"
          "Heavy, reach|\n", f);
    fclose(f);

    EQ(homebrew_load(), 2, "both homebrew entries load");

    for (i = 0, found = 0; i < MAGIC_ITEM_COUNT; i++) {
        if (!strcmp(MAGIC_ITEMS[i].name, "Hammer|of Dawn")) found = 1;
    }
    check(found, "a '|' in a magic item's name comes back", 0, 0);
    for (i = 0, found = 0; i < ITEM_COUNT; i++) {
        if (!strcmp(ITEMS[i].name, "Pole|arm")) found = 1;
    }
    check(found, "a '|' in an item's name comes back", 0, 0);

    /* Written back out, the separator must be escaped again rather than
       breaking the line it is written on. */
    EQ(homebrew_save(), 0, "the homebrew file is written");
    slurp("homebrew.txt", after, sizeof after);
    check(strstr(after, "Hammer\\pof Dawn") != NULL,
          "the name is written back escaped", 0, 0);
    check(strstr(after, "5\\\\10 gp") != NULL,
          "a backslash is written back escaped", 0, 0);

    EQ(homebrew_load(), 2, "and the file it wrote reads back");
    for (i = 0, found = 0; i < MAGIC_ITEM_COUNT; i++) {
        if (!strcmp(MAGIC_ITEMS[i].name, "Hammer|of Dawn")) found = 1;
    }
    check(found, "the name survives a second trip through the file", 0, 0);

    if (n_before) {
        f = fopen("homebrew.txt", "w");
        if (f) { fwrite(before, 1, n_before, f); fclose(f); }
    } else {
        remove("homebrew.txt");
    }
}

int main(void)
{
    printf("dndcreator self-test\n\n");
    settings_defaults(&SETTINGS);

    test_modifiers();
    test_proficiency();
    test_bruenor();
    test_hill_dwarf_and_tough();
    test_armour_class();
    test_spell_slots();
    test_artificer();
    test_pact_magic();
    test_skills_and_saves();
    test_spell_data();
    test_data_integrity();
    test_expansion_data();
    test_item_reference();
    test_option_lists();
    test_option_spells();
    test_beasts();
    test_sidekicks();
    test_homebrew_banks();
    test_magic_armour_class();
    test_magic_scores_and_speeds();
    test_life_tables();
    test_notes_and_custom_background();
    test_attacks();
    test_choice_lists();
    test_carrying_and_coins();
    test_inventory_id_spaces();
    test_awkward_text();
    test_sweep_characters();
    test_sweep_content();
    /* Last: it repoints the banks, which the checks above compare
       against the books' own tables. */
    test_homebrew_file();

    printf("\n%s\n", failures ? "FAILURES" : "all checks passed");
    return failures ? 1 : 0;
}
